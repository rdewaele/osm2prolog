# Copyright (C) 2010, 2011 Robrecht Dewaele
#
# This file is part of osm2prolog.
#
# osm2prolog is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# osm2prolog is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with osm2prolog.  If not, see <http://www.gnu.org/licenses/>.

# About this makefile:
#
# This (GNU) makefile uses the builtin rules as much as possible. The
# header dependencies for individual objects are generated by $(CC),
# this means that currently GCC is expected. (Or any other compiler
# with the same behaviour for the particular command line switches.)

CWARNINGS=-W -Wall -Wextra -Wundef -Wendif-labels -Wshadow\
					-Wpointer-arith -Wbad-function-cast -Wcast-align\
					-Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes\
					-Wnested-externs -Winline -Wdisabled-optimization\
					-Wno-missing-field-initializers
CFLAGS:=-O3 -pipe -pedantic -std=c89 $(CWARNINGS)\
	      $(shell xml2-config --cflags) $(CFLAGS)
LDLIBS:=$(shell xml2-config --libs) $(LDFLIBS)
SOURCES=src/main.c
OBJECTS=$(SOURCES:.c=.o)
MAIN=src/main
EXECUTABLE=osm2prolog

all: $(EXECUTABLE)

$(EXECUTABLE): $(MAIN)
	cp $< $@

$(MAIN): $(OBJECTS)

clean:
	@rm -f $(OBJECTS) $(EXECUTABLE) $(MAIN)
