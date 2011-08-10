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

#include <stdlib.h>

typedef
enum osmElement {
	_OSM_ELEMENT_UNSET_ = 0,
	OSM, NODE, WAY, TAG, ND, RELATION, MEMBER, ID, LAT, LON, REF, K, V, VERSION,
	CREATEDBY, NOTE,
	_OSM_ELEMENT_SIZE_
}
osmElement;

typedef
enum osmPrintMode {
	_OSM_PRINT_MODE_UNSET_ = 0,
	PL,
	TABLE,
	_OSM_PRINT_MODE_SIZE_
}
osmPrintMode;

typedef
struct osm_config {
	osmPrintMode printMode;
	char * tableprefix;
}
osm_config;
