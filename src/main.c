/* Copyright (C) 2010, 2011 Robrecht Dewaele
 *
 * This file is part of osm2prolog.
 *
 * osm2prolog is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * osm2prolog is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with osm2prolog.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>

/* TODO: check whether long long types are really 64bit at least when compiling */

/*****************
 * SAX PARSER
 *****************/

static xmlSAXHandler osm2prolog = {
	/*  internalSubsetSAXFunc internalSubset;*/
	NULL,
	/*  isStandaloneSAXFunc isStandalone;*/
	NULL,
	/*  hasInternalSubsetSAXFunc hasInternalSubset;*/
	NULL,
	/*  hasExternalSubsetSAXFunc hasExternalSubset;*/
	NULL,
	/*  resolveEntitySAXFunc resolveEntity;*/
	NULL,
	/*  getEntitySAXFunc getEntity;*/
	NULL,
	/*  entityDeclSAXFunc entityDecl;*/
	NULL,
	/*  notationDeclSAXFunc notationDecl;*/
	NULL,
	/*  attributeDeclSAXFunc attributeDecl;*/
	NULL,
	/*  elementDeclSAXFunc elementDecl;*/
	NULL,
	/*  unparsedEntityDeclSAXFunc unparsedEntityDecl;*/
	NULL,
	/*  setDocumentLocatorSAXFunc setDocumentLocator;*/
	NULL,
	/*  startDocumentSAXFunc startDocument;*/
	startDocument,
	/*  endDocumentSAXFunc endDocument;*/
	endDocument,
	/*  startElementSAXFunc startElement;*/
	startElement,
	/*  endElementSAXFunc endElement;*/
	endElement,
	/*  referenceSAXFunc reference;*/
	NULL,
	/*  charactersSAXFunc characters;*/
	NULL,
	/*  ignorableWhitespaceSAXFunc ignorableWhitespace;*/
	NULL,
	/*  processingInstructionSAXFunc processingInstruction;*/
	NULL,
	/*  commentSAXFunc comment;*/
	NULL,
	/*  warningSAXFunc warning;*/
	NULL,
	/*  errorSAXFunc error;*/
	NULL,
	/*  fatalErrorSAXFunc fatalError;*/
	NULL
};

typedef enum osmElement {
	_OSM_ELEMENT_UNSET_ = 0,
	OSM, NODE, WAY, TAG, ND, RELATION, MEMBER, ID, LAT, LON, REF, K, V, VERSION,
	CREATEDBY, NOTE,
	_OSM_ELEMENT_SIZE_
	} osmElement;
static xmlChar ** strConstants;

typedef struct parseState {
	osmElement parent;	
	long long parentid;

	size_t numways;
	const size_t maxways;
	long long * waynodeids;
} parseState;

/************************/
/* forward declarations */
/************************/
size_t findAttributes(size_t numkeys, osmElement keys[], const xmlChar ** attrs, const xmlChar ** values);

void parseOSM(const xmlChar ** attrs);
void parseNode(const xmlChar * name, parseState * state, const xmlChar ** attrs);
void parseWay(const xmlChar * name, parseState * state, const xmlChar ** attrs);
void parseND(const xmlChar * name, parseState * state, const xmlChar ** attrs);
void parseTag(const xmlChar * name, parseState * state, const xmlChar ** attrs);

xmlChar * prolog_filter_str(const xmlChar * str);
bool osmIgnoreKey(const xmlChar * keyname);



/*************/
/* callbacks */
/*************/
void startDocument(void * user_data) {
	/* "way is an ordered interconnection of at least 2 and at most 2,000[1] (API v0.6) nodes"
	 * from: http://wiki.openstreetmap.org/wiki/Ways
	 * TODO: dynamically manage memory for ways */
	const size_t maxways = 2000;
	parseState state = {_OSM_ELEMENT_UNSET_, 0, 0, maxways, xmlMalloc(maxways * sizeof(long long))};
	memcpy(user_data, &state, sizeof(state));

	fprintf(stderr, "Start Document\n");

	strConstants = xmlMalloc(_OSM_ELEMENT_SIZE_ * sizeof(xmlChar *));

	strConstants[OSM] = xmlCharStrdup("osm");
	strConstants[NODE] = xmlCharStrdup("node");
	strConstants[WAY] = xmlCharStrdup("way");
	strConstants[TAG] = xmlCharStrdup("tag");
	strConstants[ND] = xmlCharStrdup("nd");
	strConstants[MEMBER] = xmlCharStrdup("member");
	strConstants[RELATION] = xmlCharStrdup("relation");
	strConstants[ID] = xmlCharStrdup("id");
	strConstants[LAT] = xmlCharStrdup("lat");
	strConstants[LON] = xmlCharStrdup("lon");
	strConstants[REF] = xmlCharStrdup("ref");
	strConstants[K] = xmlCharStrdup("k");
	strConstants[V] = xmlCharStrdup("v");
	strConstants[VERSION] = xmlCharStrdup("version");
	strConstants[CREATEDBY] = xmlCharStrdup("created_by");
	strConstants[NOTE] = xmlCharStrdup("note");

	/* prevent swipl from complaining about the order of clauses */
	printf(":-style_check(-discontiguous).\n");
}

void endDocument(void * user_data) {
	parseState * state = user_data;
	size_t i;

	for (i = _OSM_ELEMENT_UNSET_ + 1; i < _OSM_ELEMENT_SIZE_; ++i)
		xmlFree(strConstants[i]);

	xmlFree(strConstants);

	xmlFree(state->waynodeids);

	fprintf(stderr, "End Document\n");
}

void startElement(void * user_data, const xmlChar * name, const xmlChar ** attrs) {
	parseState * state = user_data;

	/* --- accepted --- */
	/* osm (the xml root node) */
	if (xmlStrEqual(name, strConstants[OSM])) {
		parseOSM(attrs);
		return;
	}

	/* node */
	if (xmlStrEqual(name, strConstants[NODE])) {
		parseNode(name, state, attrs);
		return;
	}
	/* way */
	if (xmlStrEqual(name, strConstants[WAY])) {
		parseWay(name, state, attrs);
		return;
	}
	/* nd */
	if (xmlStrEqual(name, strConstants[ND])) {
		parseND(name, state, attrs);
		return;
	}
	/* tag */
	if (xmlStrEqual(name, strConstants[TAG])) {
		parseTag(name, state, attrs);
		return;
	}

	/* --- ignored (deliberately and explicitely) --- */

	/* relation */
	if (xmlStrEqual(name, strConstants[RELATION]))
		return;
	/* member */
	if (xmlStrEqual(name, strConstants[MEMBER]))
		return;

	/* --- unknown --- */

	fprintf(stderr, "unknown element: %s\n", name);
}

void endElement(void * user_data, const xmlChar * name) {
	parseState * state = user_data;
	size_t i;
	size_t waysmaxidx = state->numways - 1;

	/* node */
	if (xmlStrEqual(name, strConstants[NODE])) {
		state->parent = _OSM_ELEMENT_UNSET_;
		state->numways = 0;
	}

	/* way */
	if (xmlStrEqual(name, strConstants[WAY])) {
 		if (state->numways > 0) {
			printf("%s(%lli, [", name, state->parentid);
			for (i = 0; i < waysmaxidx; ++i)
				printf("%lli, ", (state->waynodeids)[i]);
			printf("%lli]).\n", state->waynodeids[waysmaxidx]);
		}
		state->parent = _OSM_ELEMENT_UNSET_;
		state->numways = 0;
		return;
	}
}



/*********************/
/* parsing functions */
/*********************/
size_t findAttributes(size_t numkeys, osmElement keys[], const xmlChar ** attrs, const xmlChar ** values) {
	const xmlChar ** attrs_idx;
	size_t i;

	size_t foundkeys = 0;

	/* no attributes or no keys: return immediately */
	if (!attrs || !attrs[0] || !attrs[1] || !numkeys)
		return 0;

	/* as the order of the keys is important, check for each of them in order */
	for (i = 0; i < numkeys; ++i) {
		attrs_idx = attrs;
		do {
			if (xmlStrEqual(strConstants[keys[i]], attrs_idx[0])) {
				values[i] = attrs_idx[1];
				++foundkeys;
				break;
			}
		}	while (*(attrs_idx += 2));
	}
	return foundkeys;
}

void parseOSM(const xmlChar ** attrs) {
	osmElement keys[] = {VERSION};
	const size_t numkeys = (sizeof(keys) / sizeof(osmElement));
	const xmlChar ** values = xmlMalloc(numkeys * sizeof(xmlChar *));

	size_t foundkeys = findAttributes(numkeys, keys, attrs, values);

	if (foundkeys == numkeys)
		fprintf(stderr, "Openstreetmap XML, version %s\n", values[0]);

	xmlFree(values);
}

void parseNode(const xmlChar * name, parseState * state, const xmlChar ** attrs) {
	size_t i;
	char * endptr = NULL;
	long long id;
	osmElement keys[] = {ID, LAT, LON};
	const size_t numkeys = (sizeof(keys) / sizeof(osmElement));
	const size_t keysmaxidx = numkeys - 1;
	const xmlChar ** values = xmlMalloc(numkeys * sizeof(xmlChar *));

	size_t foundkeys = findAttributes(numkeys, keys, attrs, values);

	/* print the tuple if it's been found */
	if (foundkeys == numkeys) {
		id = strtoll((char *)values[0], &endptr, 10);
		if ('\0' == *endptr) {
			state->parent = NODE;
			state->parentid = id;
		}
		printf("%s(", name);
		for (i = 0; i < keysmaxidx; ++i)
			printf("%s, ", values[i]);
		printf("%s).\n", values[keysmaxidx]);
	}
	else
		fprintf(stderr, "Warning: Not all required keys for record <%s> found. Ignoring %s record.\n", name, name);

	xmlFree(values);
}

void parseWay(const xmlChar * name __attribute__((unused)), parseState * state, const xmlChar ** attrs) {
	osmElement keys[] = {ID};
	char * endptr = NULL;
	long long id;
	const size_t numkeys = (sizeof(keys) / sizeof(osmElement));
	const xmlChar ** values = xmlMalloc(numkeys * sizeof(xmlChar *));

	size_t foundkeys = findAttributes(numkeys, keys, attrs, values);

	if (foundkeys == numkeys) {
		id = strtoll((char *)values[0], &endptr, 10);
		if ('\0' == *endptr) {
			state->parent = WAY;
			state->parentid = id;
		}
		else
			fprintf(stderr, "Warning: Failed to convert way ID from string to number. Ignoring way record.\n");
	}
	else
		fprintf(stderr, "Warning: Failed to find the ID for the current way record. Ignoring way record.\n");

	xmlFree(values);
}

void parseND(const xmlChar * name __attribute__((unused)), parseState * state, const xmlChar ** attrs) {
	osmElement keys[] = {REF};
	char * endptr = NULL;
	long long id;
	const size_t numkeys = (sizeof(keys) / sizeof(osmElement));
	const xmlChar ** values = xmlMalloc(numkeys * sizeof(xmlChar *));

	size_t foundkeys = findAttributes(numkeys, keys, attrs, values);

	if (foundkeys == numkeys) {
		if (state->parent == WAY) {
			id = strtoll((char *)values[0], &endptr, 10);
			if (endptr && '\0' == *endptr) {
				state->waynodeids[state->numways++] = id;
			}
			else
				fprintf(stderr, "Warning: Failed to convert ND node ID from string to number. Ignoring ND node.\n");
		}
		else
			fprintf(stderr, "Warning: Ignoring ND element outside WAY element.\n");
	}
	else
		fprintf(stderr, "Warning: Failed to find the ID for the current ND record. Ignoring ND node.\n");

	xmlFree(values);
}

/* TODO write numbers as numbers and strings as strings */
/* TODO don't hardcode the tags being ignored */
/* TODO check for other nasty things in values like newlines */
void parseTag(const xmlChar * name __attribute__((unused)), parseState * state, const xmlChar ** attrs) {
	xmlChar * prefix = NULL;
	xmlChar * key = NULL;
	xmlChar * value = NULL;
	osmElement keys[] = {K, V};
	const size_t numkeys = (sizeof(keys) / sizeof(osmElement));
	const xmlChar ** values = xmlMalloc(numkeys * sizeof(xmlChar *));

	size_t foundkeys = findAttributes(numkeys, keys, attrs, values);

	/* known tag prefixes */
	if (NODE == state->parent)
		prefix = strConstants[NODE];
	if (WAY == state->parent)
		prefix = strConstants[WAY];

	/* print the tuple if it's been found && we know how to print it && we actually want to print it */
	/* TODO: do this printing like the abstract way in parseNode (i.e. extract that as a function)*/
	if (foundkeys != numkeys && prefix) {
		fprintf(stderr,	"Warning: Not all required keys for record <%s> found. Ignoring %s record in %s record.\n",
				name, name, prefix);
	}
	else {
		if (!osmIgnoreKey(values[0])) {
			key = prolog_filter_str(values[0]);
			value = prolog_filter_str(values[1]);
			printf("%s_%s(%lld, '%s', '%s').\n",
				prefix,
				name,
				state->parentid,
				key ? key : values[0],
				value ? value : values[1]
			);
			/* free(NULL) || free(<valid pointer>) */
			xmlFree(key);
			xmlFree(value);
		}
	}
	xmlFree(values);
}


/* main */
int main(int argc, char * argv[]) {
	int error;
	parseState * state = xmlMalloc(sizeof(parseState));

	fprintf(stderr, "osm2prolog v0.2 - usage and license: see the 'README' and 'COPYING' files.\n");

	assert(argc == 2);

	error = xmlSAXUserParseFile(&osm2prolog, state, argv[1]);

	xmlFree(state);

	if (error < 0)
		return EXIT_FAILURE;
	else
		return EXIT_SUCCESS;
}

/* util functions */

/* currently changes all whitespace to space */
xmlChar * prolog_filter_str(const xmlChar * str) {
	xmlChar * retval = NULL;
	xmlChar * dest = NULL;

	/* this is expected to be a rare case, so we amortize performance by paying less for strings
	 * that don't include newlines, and a bit more for strings who do */
	if (!xmlStrchr(str, '\n')) {
		retval = dest = xmlMalloc(xmlStrlen(str) + 1);
		while('\0' != (*dest++ = (isspace(*str++) ? ' ' : *str))); /* sequence point after '?' */
	}

	return retval;
}

bool osmIgnoreKey(const xmlChar * keyname) {
	return
		xmlStrcmp(keyname, strConstants[CREATEDBY])
		|| xmlStrcmp(keyname, strConstants[NOTE])
	;
}
