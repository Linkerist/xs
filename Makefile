VERSION=0.0.1

CXX = g++
CPPFLAGS=-DVERSION=\"${VERSION}\" -D_GNU_SOURCE
CXXFLAGS = -Wall -g -O2 -std=c++11
PREFIX?=/usr/local
MANDIR?=$(PREFIX)/share/man
DATADIR?=$(PREFIX)/share/doc
BINDIR?=$(PREFIX)/bin
DEBUGGER?=

INSTALL=install
INSTALL_PROGRAM=$(INSTALL)
INSTALL_DATA=${INSTALL} -m 644

LIBS = -lcurses

OBJECTS=src/xs.o src/io.o src/misc.o src/ui.o

all: xscore

xscore: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(CCFLAGS) -o $@ $(OBJECTS) $(LIBS)

%.o: %.c config.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

shell:
	echo 'source /usr/local/share/doc/xs/examples/xs-bash.sh' >> ~/.bashrc

install: xscore
	mkdir -p $(DESTDIR)$(BINDIR)
	cp xscore $(DESTDIR)$(BINDIR)/
	chmod 755 ${DESTDIR}${BINDIR}/xscore
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	cp doc/xs.1 $(DESTDIR)$(MANDIR)/man1/
	chmod 644 ${DESTDIR}${MANDIR}/man1/xs.1
	mkdir -p $(DATADIR)/xs/examples/
	cp shell/xs-bash.sh $(DATADIR)/xs/examples/
	chmod 755 $(DATADIR)/xs/examples/xs-bash.sh

clean:
	rm -f xscore src/*.o

.PHONY: all clean install shell
