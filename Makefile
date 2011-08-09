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



#
# user modifiable variables
#

# final executable name
EXECUTABLE=osm2prolog.bin

# OpenStreetMap XML export file, for testing
OSMXML=leuven-nov-2010.xml



#
# private variables
#

# where the source tree lives
SOURCE=src

# default target to build in $(SOURCE)
SOURCE_TARGET=osm2prolog



#
# build rules.
#

# default rule: build main executable
all: $(EXECUTABLE)

# build main executable
$(EXECUTABLE):
	$(MAKE) -C $(SOURCE) $(SOURCE_TARGET)
	cp $(SOURCE)/$(SOURCE_TARGET) $(EXECUTABLE)

# clean: clean source dir, then self
clean:
	$(MAKE) -C $(SOURCE) $@
	rm -f $(EXECUTABLE)

# run osm2prolog through valgrind using the supplied osm xml sample
valgrind: $(EXECUTABLE)
	valgrind --leak-check=full ./$(EXECUTABLE) $(OSMXML) > /dev/null

.PHONY: valgrind $(EXECUTABLE)
