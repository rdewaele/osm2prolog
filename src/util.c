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

#include "util.h"
#include "types.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <libxml/xmlmemory.h>
#include <libxml/xmlstring.h>

xmlChar ** strConstants;

static bool prolog_is_nasty_char(const xmlChar c);

static inline bool prolog_is_nasty_char(const xmlChar c) {
	return isspace(c)
		|| '\'' == c;
}

/* TODO check for other nasty things in values like newlines and single quotes */
/* TODO we could also escape single quotes instead of overwriting them, but it would matter what format we print to */
xmlChar * prolog_filter_str(const xmlChar * str) {
	xmlChar * retval = NULL;
	xmlChar * dest = NULL;

	retval = dest = xmlMalloc(xmlStrlen(str) + 1);
	/* sequence point after '?' which is a sub expression, so I don't think it's
	 * defined behaviour to post increment dest in line. */
	while('\0' != (*dest = (prolog_is_nasty_char(*str) ? ' ' : *str))) {
		++dest;
		++str;
	}

	return retval;
}



/* TODO don't hardcode the tags being ignored */
bool osmIgnoreKey(const xmlChar * keyname) {
	return (!keyname)
		|| 0 == xmlStrcmp(keyname, strConstants[CREATEDBY])
		|| 0 == xmlStrcmp(keyname, strConstants[NOTE])
		;
}



void osm2prolog_init(void) {
	strConstants = xmlMalloc(_OSM_ELEMENT_SIZE_ * sizeof(xmlChar *));

	strConstants[_OSM_ELEMENT_UNSET_] = NULL;

	strConstants[OSM] = xmlCharStrdup("osm");
	strConstants[NODE] = xmlCharStrdup("node");
	strConstants[WAY] = xmlCharStrdup("way");
	strConstants[TAG] = xmlCharStrdup("tag");
	strConstants[ND] = xmlCharStrdup("nd");
	strConstants[RELATION] = xmlCharStrdup("relation");
	strConstants[MEMBER] = xmlCharStrdup("member");
	strConstants[ID] = xmlCharStrdup("id");
	strConstants[LAT] = xmlCharStrdup("lat");
	strConstants[LON] = xmlCharStrdup("lon");
	strConstants[REF] = xmlCharStrdup("ref");
	strConstants[K] = xmlCharStrdup("k");
	strConstants[V] = xmlCharStrdup("v");
	strConstants[VERSION] = xmlCharStrdup("version");
	strConstants[CREATEDBY] = xmlCharStrdup("created_by");
	strConstants[NOTE] = xmlCharStrdup("note");
}

void osm2prolog_cleanup(void) {
	osmElement i;

	for (i = 0; i < _OSM_ELEMENT_SIZE_; ++i)
		xmlFree(strConstants[i]);

	xmlFree(strConstants);
}

parseState * osm2prolog_createParseState (void) {
	/* "way is an ordered interconnection of at least 2 and at most 2,000[1] (API v0.6) nodes"
	 * from: http://wiki.openstreetmap.org/wiki/Ways
	 * TODO: dynamically manage memory for ways */
	const size_t maxways = 2000;
	parseState * state = xmlMalloc(sizeof(parseState));
	parseState src_state = {
		_OSM_ELEMENT_UNSET_,
		0,
		true,
		NULL,
		NULL,
		0,
		maxways,
		xmlMalloc(maxways * sizeof(int_least64_t)),
		true,
		NULL,
		NULL,
		NULL,
		_OSM_PRINT_MODE_UNSET_,
		NULL,
		NULL,
		NULL,
		NULL
	};
	memcpy(state, &src_state, sizeof(src_state));
	return state;
}

void osm2prolog_freeParseState(parseState * state) {
	xmlFree(state->waynodeids);
	xmlFree(state);
}
