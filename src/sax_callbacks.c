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

#include "sax_callbacks.h"
#include "types.h"
#include "util.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>

xmlSAXHandler osm2prolog = {
	NULL, /* internalSubsetSAXFunc internalSubset */
	NULL, /* isStandaloneSAXFunc isStandalone */
	NULL, /* hasInternalSubsetSAXFunc hasInternalSubset */
	NULL, /* hasExternalSubsetSAXFunc hasExternalSubset */
	NULL, /* resolveEntitySAXFunc resolveEntity */
	NULL, /* getEntitySAXFunc getEntity */
	NULL, /* entityDeclSAXFunc entityDecl */
	NULL, /* notationDeclSAXFunc notationDecl */
	NULL, /* attributeDeclSAXFunc attributeDecl */
	NULL, /* elementDeclSAXFunc elementDecl */
	NULL, /* unparsedEntityDeclSAXFunc unparsedEntityDecl */
	NULL, /* setDocumentLocatorSAXFunc setDocumentLocator */
	startDocument, /* startDocumentSAXFunc startDocument */
	endDocument, /* endDocumentSAXFunc endDocument */
	startElement, /* startElementSAXFunc startElement */
	endElement, /* endElementSAXFunc endElement */
	NULL, /* referenceSAXFunc reference */
	NULL, /* charactersSAXFunc characters */
	NULL, /* ignorableWhitespaceSAXFunc ignorableWhitespace */
	NULL, /* processingInstructionSAXFunc processingInstruction */
	NULL, /* commentSAXFunc comment */
	NULL, /* warningSAXFunc warning */
	NULL, /* errorSAXFunc error */
	NULL, /* fatalErrorSAXFunc fatalError */
};

/************************/
/* forward declarations */
/************************/
static size_t findAttributes(size_t numkeys, osmElement keys[], const xmlChar ** attrs, const xmlChar ** values);

static void parseOSM(const xmlChar ** attrs);
static void parseNode(const xmlChar * name, parseState * state, const xmlChar ** attrs);
static void parseWay(const xmlChar * name, parseState * state, const xmlChar ** attrs);
static void parseND(const xmlChar * name, parseState * state, const xmlChar ** attrs);
static void parseTag(const xmlChar * name, parseState * state, const xmlChar ** attrs);

static void printNode(const xmlChar * name, parseState * state);
static void printWay(const xmlChar * name, parseState * state);
static void printTag(const xmlChar * name, parseState * state);

static bool osm_strtoimax(const xmlChar * str, int_least64_t * num);
static bool validDouble(const xmlChar * str);

/*************/
/* callbacks */
/*************/
void startDocument(void * user_data) {
	parseState * state = user_data;

	/* Technically, these values are probably already set, but that's because
	 * these are general sane values. We don't wan't to _depend_ on that though. */
	state->parent = _OSM_ELEMENT_UNSET_;
	state->lat = NULL;
	state->lon = NULL;
	state->badnode = true;
	state->parentid = 0;
	state->numways = 0;
	state->tagprefix = NULL;
	state->tagkey = NULL;
	state->tagvalue = NULL;

	osm2prolog_init();

	/* prevent swipl from complaining about the order of clauses */
	printf(":-style_check(-discontiguous).\n");

	fprintf(stderr, "Start Document\n");
}

void endDocument(void * user_data __attribute__((unused))) {
	fprintf(stderr, "End Document\n");

	osm2prolog_cleanup();
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

	/* node */
	if (xmlStrEqual(name, strConstants[NODE]) && !state->badnode) {
		printNode(name, state);
		state->parent = _OSM_ELEMENT_UNSET_;
		xmlFree(state->lon);
		xmlFree(state->lat);
	}

	/* way */
	if (xmlStrEqual(name, strConstants[WAY])) {
 		if (0 == state->numways)
			fprintf(stderr, "Warning: way element doesn't contain nodes. Ignoring way.");
		else
			printWay(name, state);

		state->parent = _OSM_ELEMENT_UNSET_;
		state->numways = 0;
		return;
	}

	/* nd - is only handled as a child of way*/

	/* tag */	
	if (xmlStrEqual(name, strConstants[TAG])) {
		if (state->tagkey && !osmIgnoreKey(state->tagkey))
			printTag(name, state);

		xmlFree(state->tagkey);
		state->tagkey = NULL;
		xmlFree(state->tagvalue);
		state->tagvalue = NULL;
	}
}



/*********************/
/* parsing functions */
/*********************/
static size_t findAttributes(size_t numkeys, osmElement keys[], const xmlChar ** attrs, const xmlChar ** values) {
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

static void parseOSM(const xmlChar ** attrs) {
	osmElement keys[] = {VERSION};
	const size_t numkeys = (sizeof(keys) / sizeof(osmElement));
	const xmlChar ** values = xmlMalloc(numkeys * sizeof(xmlChar *));

	size_t foundkeys = findAttributes(numkeys, keys, attrs, values);

	if (foundkeys == numkeys)
		fprintf(stderr, "Openstreetmap XML, version %s\n", values[0]);

	xmlFree(values);
}

static void parseNode(const xmlChar * name __attribute__((unused)), parseState * state, const xmlChar ** attrs) {
	osmElement keys[] = {ID, LAT, LON};
	const size_t numkeys = (sizeof(keys) / sizeof(osmElement));
	const xmlChar ** values = xmlMalloc(numkeys * sizeof(xmlChar *));

	size_t foundkeys = findAttributes(numkeys, keys, attrs, values);

	state->badnode = true;

	/* save the tuple if it's conform to what we expect */
	if (foundkeys != numkeys)
		fprintf(stderr, "Warning: Not all required keys for node record found. Ignoring node record.\n");
	else {
		if (!(
				osm_strtoimax(values[0], &(state->parentid))
				&& validDouble(values[1])
				&& validDouble(values[2])
				))
			fprintf(stderr, "Warning: Failed to convert node ID, LAT or LON from string to number. Ignoring node record.\n");
		else
			state->parent = NODE;
			state->lat = xmlStrdup(values[1]);
			state->lon = xmlStrdup(values[2]);
			state->badnode = false;
		}

	xmlFree(values);
}

static void parseWay(const xmlChar * name __attribute__((unused)), parseState * state, const xmlChar ** attrs) {
	osmElement keys[] = {ID};
	const size_t numkeys = (sizeof(keys) / sizeof(osmElement));
	const xmlChar ** values = xmlMalloc(numkeys * sizeof(xmlChar *));

	size_t foundkeys = findAttributes(numkeys, keys, attrs, values);

	if (foundkeys != numkeys)
		fprintf(stderr, "Warning: Failed to find the ID for the current way record. Ignoring way record.\n");
	else {
		if (!osm_strtoimax(values[0], &(state->parentid)))
			fprintf(stderr, "Warning: Failed to convert way ID from string to number. Ignoring way record.\n");
		else
			state->parent = WAY;
	}

	xmlFree(values);
}

void parseND(const xmlChar * name __attribute__((unused)), parseState * state, const xmlChar ** attrs) {
	osmElement keys[] = {REF};
	const size_t numkeys = (sizeof(keys) / sizeof(osmElement));
	const xmlChar ** values = xmlMalloc(numkeys * sizeof(xmlChar *));

	size_t foundkeys = findAttributes(numkeys, keys, attrs, values);

	if (foundkeys != numkeys)
		fprintf(stderr, "Warning: Failed to find the REF (reference node) for the current ND record. Ignoring ND node.\n");
	else {
		if (state->parent != WAY)
			fprintf(stderr, "Warning: Ignoring ND element outside WAY element.\n");
		else {
			if (!osm_strtoimax(values[0], &(state->waynodeids[state->numways++])))
				fprintf(stderr, "Warning: Failed to convert ND node ID from string to number. Ignoring ND node.\n");
			/* else: ND must not set parent so no action here */
		}
	}

	xmlFree(values);
}

static void parseTag(const xmlChar * name __attribute__((unused)), parseState * state, const xmlChar ** attrs) {
	osmElement keys[] = {K, V};
	const size_t numkeys = (sizeof(keys) / sizeof(osmElement));
	const xmlChar ** values = xmlMalloc(numkeys * sizeof(xmlChar *));

	size_t foundkeys = findAttributes(numkeys, keys, attrs, values);

	state->tagprefix = NULL;
	state->tagkey = NULL;
	state->tagvalue = NULL;

	/* known tag prefixes */
	if (NODE == state->parent)
		state->tagprefix = strConstants[NODE];
	if (WAY == state->parent)
		state->tagprefix = strConstants[WAY];

	if (foundkeys != numkeys && state->tagprefix)
		fprintf(stderr,	"Warning: Not all required keys for record <%s> found. Ignoring %s record in %s record.\n",
				name, name, state->tagprefix);
	else
		state->tagkey = xmlStrdup(values[0]);
		state->tagvalue = xmlStrdup(values[1]);

	xmlFree(values);
}



/************/
/* PRINTING */
/************/

static void printWay(const xmlChar * name, parseState * state) {
	size_t i = 0;
	size_t waysmaxidx = state->numways - 1;
	printf("%s(%" PRIdLEAST64 ", [", name, state->parentid);
	while(i < waysmaxidx) printf("%" PRIdLEAST64 ", ", (state->waynodeids)[i++]);
	printf("%" PRIdLEAST64 "]).\n", state->waynodeids[waysmaxidx]);
}

static void printNode(const xmlChar * name, parseState * state) {
	printf("%s(%" PRIdLEAST64 ", %s, %s).\n", name, state->parentid, state->lat, state->lon);
}	

static void printTag(const xmlChar * name, parseState * state) {
	xmlChar * key = NULL;
	xmlChar * value = NULL;

	key = prolog_filter_str(state->tagkey);
	value = prolog_filter_str(state->tagvalue);

	printf("%s_%s(%" PRIdLEAST64 ", '%s', '%s').\n",
		state->tagprefix,
		name,
		state->parentid,
		key ? key : state->tagkey,
		value ? value : state->tagvalue
	);
	/* free(NULL) || free(<valid pointer>) */
	xmlFree(key);
	xmlFree(value);
}



/********/
/* UTIL */
/********/

static bool osm_strtoimax(const xmlChar * str, int_least64_t * num) {
	char * endptr = NULL;
	int_least64_t id = strtoimax((char *)str, &endptr, 10);
	if ('\0' == *endptr) {
		*num = id;
		return true;
	}
	else
		return false;
}

static bool validDouble(const xmlChar * str) {
	char * endptr = NULL;
	(void)strtod((char *)str, &endptr);
	return '\0' == *endptr;
}
