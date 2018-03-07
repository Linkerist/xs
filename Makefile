VERSION=0.0.1

CXX = g++
CPPFLAGS=-DVERSION=\"${VERSION}\" -D_GNU_SOURCE
CXXFLAGS = -Wall -g -O2 -std=c++11
# PREFIX?=/usr/local
PREFIX?=/home/qianzichen/proj/xs/xs/__install
MANDIR?=$(PREFIX)/share/man
BINDIR?=$(PREFIX)/bin
DEBUGGER?=

INSTALL=install
INSTALL_PROGRAM=$(INSTALL)
INSTALL_DATA=${INSTALL} -m 644

LIBS = -lcurses

OBJECTS=src/xs.o src/io.o src/misc.o src/ui.o

all: xs

xs: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(CCFLAGS) -o $@ $(OBJECTS) $(LIBS)

%.o: %.c config.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

install: xs
	mkdir -p $(DESTDIR)$(BINDIR)
	cp xs $(DESTDIR)$(BINDIR)/
	chmod 755 ${DESTDIR}${BINDIR}/xs
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	cp doc/xs.1 $(DESTDIR)$(MANDIR)/man1/
	chmod 644 ${DESTDIR}${MANDIR}/man1/xs.1

clean:
	rm -f xs src/*.o

.PHONY: all clean install
