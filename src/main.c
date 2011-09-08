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
#include <errno.h>
#include <string.h>

/* TODO decent option parsing (libpopt or so) */

void setPrintConfig(char * argv[], parseState * state);
char * strconcat(const char * prefix, const char * infix, const char * suffix);
FILE * openPrintFile(const char * prefix, const char * suffix);

/* main */
int main(int argc, char * argv[]) {
	int error;
	char * xmlfilename = NULL;

	parseState * state = osm2prolog_createParseState();

	fprintf(stderr, "osm2prolog v0.2 - usage and license: see the 'README' and 'COPYING' files.\n");

	/* exec <osm xml filename> || exec -tbl <filename prefix> <osm xml filename> */
	assert(argc == 2 || argc == 4);

	state->printMode = PL;
	if (argc == 4) {
		fprintf(stderr, "Note: %s produces completely unsorted tables. "
				"If you would like to have the table sorted according to some column, "
				"something amongst these lines might prove to be useful:\n"
				"\tsort -s -t\"$(echo -e '\t')\" -k1n,1\n",
				argv[0]);
		setPrintConfig(argv, state);
		xmlfilename = argv[3];
	}

	if (argc == 2)
		xmlfilename = argv[1];

	error = xmlSAXUserParseFile(&osm2prolog, state, xmlfilename);
	xmlCleanupParser();
	osm2prolog_freeParseState(state);

	if (error < 0)
		return EXIT_FAILURE;
	else
		return EXIT_SUCCESS;
}

void setPrintConfig(char * argv[], parseState * state) {
	/* argv: exec -tbl <filename prefix> <osm xml filename> */
	const char * prefix = argv[2];

	if (0 == strcmp(argv[1], "-tbl")) {
		state->printMode = TABLE;

		state->node_file = openPrintFile(prefix, "node");
		state->way_file = openPrintFile(prefix, "way");
		state->nodetag_file = openPrintFile(prefix, "nodetag");
		state->waytag_file = openPrintFile(prefix, "waytag");
	}
	else {
		fprintf(stderr, "usage: %s [-tbl <filename prefix>] <input.xml>\n", argv[0]);
		fprintf(stderr, "was invoked as \"'%s' '%s' '%s' '%s'\"\n'", argv[0], argv[1], argv[2], argv[3]);
		osm2prolog_freeParseState(state);
		exit(EXIT_FAILURE);
	}
}

char * strconcat(const char * prefix, const char * infix, const char * suffix) {
	const size_t destlen = strlen(prefix) + strlen(infix) + strlen(suffix) + 1;
	char * dest;
	char * ret;
	ret = dest = malloc(destlen);
	while('\0' != (*dest++ = *prefix++));
	dest--; /* overwrite nul char */
	while('\0' != (*dest++ = *infix++));
	dest--; /* overwrite nul char */
	while('\0' != (*dest++ = *suffix++));
	return ret;
}

FILE * openPrintFile(const char * prefix, const char * suffix) {
	char * filename = strconcat(prefix, "_", suffix);
	FILE * filep = fopen(filename, "w");
	free(filename);
	if (!filep)
		perror("openPrintFile");
	return filep;
}
