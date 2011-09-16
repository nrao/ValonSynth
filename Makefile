# Copyright (C) 2011 Associated Universities, Inc. Washington DC, USA.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
# 
# Correspondence concerning GBT software should be addressed as follows:
#    GBT Operations
#    National Radio Astronomy Observatory
#    P. O. Box 2
#    Green Bank, WV 24944-0002 USA

CC = g++
AR = ar
DOXY = doxygen
CFLAGS = -c -Wall -fPIC -DLINUX
LDFLAGS = 
SOURCES = ValonSynth.cc Serial.cc
OBJECTS = $(SOURCES:.cc=.o)
PLATFORM = LINUX
STARGET = libValonSynth.a
DTARGET = libValonSynth.so

all: $(SOURCES) $(STARGET) $(DTARGET)

.cc.o:
	$(CC) $(CFLAGS) $< -o $@

$(STARGET): $(OBJECTS)
	$(AR) rcs $@ $^

$(DTARGET): $(OBJECTS)
	$(CC) -shared $(LDFLAGS) $^ -o $@

.PHONY: docs
docs:
	$(DOXY) Doxyfile

.PHONY: clean
clean:
	rm -rf $(OBJECTS)

.PHONY: clobber
clobber: clean
	rm -rf $(STARGET) $(DTARGET)