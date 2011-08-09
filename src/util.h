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

#pragma once

#include "types.h"

#include <stdbool.h>
#include <stdint.h>
#include <libxml/xmlstring.h>

/* represents state while parsing OSM XML data */
typedef
struct parseState {
	osmElement parent;	
	int_least64_t parentid;

	size_t numways;
	const size_t maxways;
	int_least64_t * waynodeids;
}
parseState;

/* maps osmElements to strings */
extern xmlChar ** strConstants;

/* initializes osm2prolog structures */
void osm2prolog_init(void);

/* frees osm2prolog structures */
void osm2prolog_cleanup(void);

/* creates a parseState object */
parseState * osm2prolog_createParseState(void);

/* free a parseState object */
void osm2prolog_freeParseState(parseState * state);

/* (This comment might be out of sync.)
 * currently changes all whitespace to space */
xmlChar * prolog_filter_str(const xmlChar * str);

/* (This comment might be out of sync.)
 * currently ignores 'created_by' and 'note' tags */
bool osmIgnoreKey(const xmlChar * keyname);
