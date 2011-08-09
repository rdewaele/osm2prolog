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

#include <assert.h>

/* main */
int main(int argc, char * argv[]) {
	int error;

	parseState * state = osm2prolog_createParseState();

	fprintf(stderr, "osm2prolog v0.2 - usage and license: see the 'README' and 'COPYING' files.\n");

	assert(argc == 2);

	error = xmlSAXUserParseFile(&osm2prolog, state, argv[1]);
	xmlCleanupParser();

	osm2prolog_freeParseState(state);

	if (error < 0)
		return EXIT_FAILURE;
	else
		return EXIT_SUCCESS;
}
