# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.

PREFIX = /usr
DATA = /share
BIN = /bin
PKGNAME = unstickpixels
COMMAND = unstickpixels
LICENSES = $(PREFIX)$(DATA)


all: doc

doc: info

info: unstickpixels.info.gz

%.info.gz: info/%.texinfo
	makeinfo "$<"
	gzip -9 -f "$*.info"

install: install-cmd install-license install-info

install-cmd:
	install -dm755 "$(DESTDIR)$(PREFIX)$(BIN)"
	install -m755 unstickpixels "$(DESTDIR)$(PREFIX)$(BIN)/$(COMMAND)"

install-license:
	install -dm755 "$(DESTDIR)$(LICENSES)/$(PKGNAME)"
	install -m644 COPYING LICENSE "$(DESTDIR)$(LICENSES)/$(PKGNAME)"

install-info: unstickpixels.info.gz
	install -dm755 "$(DESTDIR)$(PREFIX)$(DATA)/info"
	install -m644 unstickpixels.info.gz "$(DESTDIR)$(PREFIX)$(DATA)/info/$(PKGNAME).info.gz"

uninstall:
	-rm -- "$(DESTDIR)$(PREFIX)$(BIN)/$(COMMAND)"
	-rm -- "$(DESTDIR)$(LICENSES)/$(PKGNAME)/COPYING"
	-rm -- "$(DESTDIR)$(LICENSES)/$(PKGNAME)/LICENSE"
	-rmdir -- "$(DESTDIR)$(LICENSES)/$(PKGNAME)"
	-rm -- "$(DESTDIR)$(PREFIX)$(DATA)/info/$(PKGNAME).info.gz"

.PHONY: clean
clean:
	-rm -f unstickpixels.info.gz

