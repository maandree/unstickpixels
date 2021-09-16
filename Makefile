.POSIX:

CONFIGFILE = config.mk
include $(CONFIGFILE)


all: unstickpixels
unstickpixels.o: unstickpixels.c arg.h

.c.o:
	$(CC) -c -o $@ $< $(CFLAGS) $(CPPFLAGS)

.o:
	$(CC) -o $@ $< $(LDFLAGS)

install: unstickpixels
	mkdir -p -- "$(DESTDIR)$(PREFIX)/bin/"
	mkdir -p -- "$(DESTDIR)$(MANPREFIX)/man1/"
	cp -- unstickpixels "$(DESTDIR)$(PREFIX)/bin/"
	cp -- unstickpixels.1 "$(DESTDIR)$(MANPREFIX)/man1/"

uninstall:
	-rm -f -- "$(DESTDIR)$(PREFIX)/bin/unstickpixels"
	-rm -f -- "$(DESTDIR)$(MANPREFIX)/man1/unstickpixels.1"

clean:
	-rm -f -- unstickpixels *.o *.su

.SUFFIXES:
.SUFFIXES: .o .c

.PHONY: all install uninstall clean
