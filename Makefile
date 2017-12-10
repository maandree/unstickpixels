.POSIX:

CONFIGFILE = config.mk
include $(CONFIGFILE)

all: unstickpixels

unstickpixels: unstickpixels.o
	$(CC) -o $@ unstickpixels.o $(LDFLAGS)

unstickpixels.o: unstickpixels.c arg.h

install: unstickpixels
	mkdir -p -- "$(DESTDIR)$(PREFIX)/bin/"
	mkdir -p -- "$(DESTDIR)$(MANPREFIX)/man1/"
	mkdir -p -- "$(DESTDIR)$(PREFIX)/share/licenses/unstickpixels"
	cp -- unstickpixels "$(DESTDIR)$(PREFIX)/bin/"
	cp -- unstickpixels.1 "$(DESTDIR)$(MANPREFIX)/man1/"
	cp -- LICENSE "$(DESTDIR)$(PREFIX)/share/licenses/unstickpixels"

uninstall:
	-rm -f -- "$(DESTDIR)$(PREFIX)/bin/unstickpixels"
	-rm -f -- "$(DESTDIR)$(MANPREFIX)/man1/unstickpixels.1"
	-rm -rf -- "$(DESTDIR)$(PREFIX)/share/licenses/unstickpixels"

clean:
	-rm -f -- unstickpixels *.o

.SUFFIXES:
.SUFFIXES: .o .c

.PHONY: all install uninstall clean
