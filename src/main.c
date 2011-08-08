/* Copyright (C) 2010 Robrecht Dewaele
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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#ifdef LIBXML_TREE_ENABLED

void osm2pl(xmlNode *);

void prologPrintFact(const char *, char *, bool);
char * getValues(xmlNodePtr, const char **, size_t);
char * parseNamedKV(xmlNodePtr);

void parseNode(xmlNodePtr);
void parseNodeTag(const char *, xmlNodePtr);
void parseWay(xmlNodePtr);
void parseWayTag(const char *, xmlNodePtr);

char * getNd(xmlNodePtr);

/* 
 * convert an OpenStreetMap XML file to a prolog DB 
 */
void osm2pl(xmlNodePtr osm_root) {
	xmlNode * osm_current = osm_root->xmlChildrenNode;

	while(osm_current) {
		if (!xmlStrcmp(osm_current->name, (const xmlChar *)"node"))
			parseNode(osm_current);

		if (!xmlStrcmp(osm_current->name, (const xmlChar *)"way"))
			parseWay(osm_current);

		osm_current = osm_current->next;
	}
}


void prologPrintFact(const char * name, char * data, bool freeData) {
	printf("%s(%s).\n", name, data);
	if (freeData)
		free(data);
}

/* 
 * convert xml key/value pairs to a string of values, where the values are
 * separated by commas 
 */
char * getValues(xmlNodePtr osm_node, const char * keys[], size_t len) {
	size_t i;
	double ignoredouble = 0;
	char * nbrCheck = NULL;
	char * pos = NULL;
	char * read = NULL;
	size_t buf_free = 512;
	int written = 0;
	char * buf = malloc(buf_free * sizeof(xmlChar));
	char * buf_start = buf;

	len /= sizeof(xmlChar *);

	/* read the values and abort on incorrect OSM nodes */
	for (i = 0; i < len; ++i) {
		if (!(read = (char *)xmlGetProp(osm_node, (xmlChar *)keys[i])))
			exit(1);

		if (buf_free - 10 > strlen(read)) { /* TODO fix possible overflow in a decent way :) */
			if ((ignoredouble = strtod(read, &nbrCheck)), *nbrCheck != '\0') {
				while((pos = strchr(read, '\'')))
					*pos = ' ';
				written = sprintf(buf, "'%s', ", read);
			}
			else
				written = sprintf(buf, "%s, ", read);
			buf += written;
			buf_free -= written;
		}
		else
			fprintf(stderr, "PROBLEM?\n");
		xmlFree(read);
	}
	/* erase the last ", " from buffer */
	*(buf - 2) = '\0';
	return buf_start;
}

/* 
 * parse an _OpenStreetMap_ key/value style node
 */
char * parseNamedKV(xmlNodePtr osm_node_tag) {
	static const char * osm_node_tag_keys[] = {"k", "v"};
	static const size_t numkeys = sizeof(osm_node_tag_keys);

	return getValues(osm_node_tag, osm_node_tag_keys, numkeys);
}

/*
 * parse an OpenStreetMap root->node
 */
void parseNode(xmlNodePtr osm_node) {
	static const char * osm_node_keys[] = {"id", "lat", "lon"};
	static const size_t numkeys = sizeof(osm_node_keys);
	static const char * osm_node_key_ID[] = {"id"};
	char * nodeID = getValues(osm_node, osm_node_key_ID, sizeof(osm_node_key_ID));
	xmlNodePtr osm_node_current = NULL;

	prologPrintFact("node", getValues(osm_node, osm_node_keys, numkeys), true);

	/* now process OSM node->children */
	osm_node_current = osm_node->xmlChildrenNode;
	while(osm_node_current) {
		if (!xmlStrcmp(osm_node_current->name, (const xmlChar *)"tag"))
			parseNodeTag(nodeID, osm_node_current);
		osm_node_current = osm_node_current->next;
	}
	xmlFree(nodeID);
}

void parseNodeTag(const char * nodeID, xmlNodePtr osm_node_tag) {
	const char * formatStr = "%s, %s";
	char * kv = parseNamedKV(osm_node_tag);
	char * buf = malloc(strlen(formatStr) + strlen(nodeID) + strlen(kv) + 1);

	sprintf(buf, formatStr, nodeID, kv);
	xmlFree(kv);
	prologPrintFact("node_tag", buf, true);
}

/*
 * return the reference field from an OpenStreetMap
 * root->way->Nd
 */
char * getNd(xmlNodePtr osm_way_nd) {
	static const char * osm_way_nd_keys[] = {"ref"};
	static const size_t numkeys = sizeof(osm_way_nd_keys);
	return getValues(osm_way_nd, osm_way_nd_keys, numkeys);
}

/* 
 * parse an OpenStreetMap root->way
 */
void parseWay(xmlNodePtr osm_way) {
	static const char * osm_way_keys[] = {"id"};
	static const size_t numkeys = sizeof(osm_way_keys);

	char * read = NULL;
	char * wayID = NULL;
	size_t buf_free = 16 * 1024; /* can be big ish */
	int written = 0;
	char * buf = malloc(buf_free * sizeof(xmlChar));
	char * buf_start = buf;
	xmlNode * osm_way_current;

	wayID = read = getValues(osm_way, osm_way_keys, numkeys);
	if (buf_free > strlen(read)) {
		written = sprintf(buf, "%s, [", read);
		buf += written;
		buf_free -= written;
	}
	else
		fprintf(stderr, "PROBLEM?\n");

	/* now process OSM way->children */
	osm_way_current = osm_way->xmlChildrenNode;
	while(osm_way_current) {
		if (!xmlStrcmp(osm_way_current->name, (const xmlChar *)"tag"))
			parseWayTag(wayID, osm_way_current);
		else if (!xmlStrcmp(osm_way_current->name, (const xmlChar *)"nd")) {
			read = getNd(osm_way_current);
			if (buf_free > strlen(read)) {
				written = sprintf(buf, "%s, ", read);
				buf += written;
				buf_free -= written;
				xmlFree(read);
			}
			else
				fprintf(stderr, "PROBLEM2?\n");
		}
		osm_way_current = osm_way_current->next;
	}
	sprintf((buf - 2), "]");
	prologPrintFact("way", buf_start, true);
	xmlFree(wayID);
}

/*
 * parse an OpenStreetMap root->way->tag
 */
void parseWayTag(const char * wayID, xmlNodePtr osm_way_tag) {
	const char * formatStr = "%s, %s";
	char * kv = parseNamedKV(osm_way_tag);
	char * buf = malloc(strlen(formatStr) + strlen(wayID) + strlen(kv) + 1);

	sprintf(buf, formatStr, wayID, kv);
	xmlFree(kv);
	prologPrintFact("way_tag", buf, true);
}

/* 
 * main
 */
int main(int argc, char **argv) {
	xmlDoc * doc = NULL;
	xmlNodePtr root_element = NULL;

	if (argc != 2) {
		fprintf(stderr,"usage: %s <openstreetmapdump.xml>\n", argv[0]);
		return(1);
	}

	LIBXML_TEST_VERSION;

	/*parse the file and get the DOM */
	doc = xmlReadFile(argv[1], NULL, 0);

	if (doc == NULL) {
		printf("error: could not parse file %s\n", argv[1]);
		exit(1);
	}

	/* Get the root element node */
	root_element = xmlDocGetRootElement(doc);

	/* Check for the right type of document */
	if (xmlStrcmp(root_element->name, (const xmlChar *)"osm")) {
		fprintf(stderr,"error: XML root is not an OpenStreetMap node.\n");
		exit(1);
	}

	printf(":-style_check(-discontiguous).\n\n");
	osm2pl(root_element);

	xmlFreeDoc(doc);
	xmlCleanupParser();

	return 0;
}
#else
int main(void) {
	fprintf(stderr, "Tree support not compiled in\n");
	exit(1);
}
#endif
