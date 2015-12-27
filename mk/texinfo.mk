# Copyright (C) 2015  Mattias Andrée <maandree@member.fsf.org>
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


# TODO ((support translations))
#=== These rules are used for Texinfo manuals. ===#


# Enables the rules:
#   info          Build info manual
#   dvi           Build DVI manual
#   pdf           Build PDF manual
#   ps            Build PostScript manual
#   html          Build HTML manual
#   install-info  Install info manual
#   install-dvi   Install DVI manual
#   install-pdf   Install PDF manual
#   install-ps    Install PostScript manual
#   install-html  Install HTML manual
# 
# This file is ignored unless
# _HAVE_TEXINFO_MANUAL is defined.
# 
# This file can only build output for
# one Texinfo manual. This manual must
# be named doc/info/$(_PROJECT).texinfo.
# Additional sourced are set by specifing
# how man directories deep doc/info nests
# in the variable _TEXINFO_DIRLEVELS.
# 
# If the info manual splits, specify the
# split-number, of the file with the highest
# split-number, in the variable _INFOPARTS.
# 
# If the project has a logo, _LOGO should
# name the suffixless basename of the SVG
# files that contains the logo. This file
# must be located in doc/.
# 
# _HTML_FILES should the basename (with
# suffix) of all files generated by `html`.
# 
# The user may define TEXINFO_FLAGS that
# adds additional flags when compiling
# DVI, PDF, and PostScript manuals. The
# user may also define INFO_FLAGS that
# adds additional flags when compiling
# info and HTML manuals.


ifdef _HAVE_TEXINFO_MANUAL


# WHEN TO BUILD, INSTALL, AND UNINSTALL:

all: info
everything: info dvi pdf ps html
doc: info dvi pdf ps html
install: install-info
install-everything: install-info install-dvi install-pdf install-ps install-html
install-doc: install-info install-dvi install-pdf install-ps install-html
uninstall: uninstall-info uninstall-dvi uninstall-pdf uninstall-ps uninstall-html


# HELP VARIABLES:

# Files from which the Texinfo manuals are built.
ifdef _TEXINFO_DIRLEVELS
ifeq ($(_TEXINFO_DIRLEVELS),1)
__TEXI_SRC_ = *.texinfo
endif
ifeq ($(_TEXINFO_DIRLEVELS),2)
__TEXI_SRC_ = *.texinfo */*.texinfo
endif
ifeq ($(_TEXINFO_DIRLEVELS),3)
__TEXI_SRC_ = *.texinfo */*.texinfo */*/*.texinfo
endif
ifneq ($(_TEXINFO_DIRLEVELS),1)
ifneq ($(_TEXINFO_DIRLEVELS),2)
ifneq ($(_TEXINFO_DIRLEVELS),3)
__TEXI_SRC_ = $(foreach W,$(shell $(SEQ) $(_TEXINFO_DIRLEVELS) | while read n; do $(ECHO) $$($(SEQ) $$n)" " | $(SED) 's/[^ ]* /\/\*/g'; done | $(XARGS) $(ECHO)),$(shell $(ECHO) $(W).texinfo | $(SED) 's/^.//'))
endif
endif
endif
__TEXI_SRC = $(foreach S,$(__TEXI_SRC_),$(foreach F,$(shell cd $(v)doc/info && $(ECHO) $(S)),aux/doc/$(F)))
endif

# Split parts of the info manual.
ifdef _INFOPARTS
ifneq ($(_INFOPARTS),0)
__INFOPARTS = $(shell $(SEQ))
endif
endif

# Flags for TeX processed output.
__TEXINFO_FLAGS = $(TEXINFO_FLAGS)
ifdef _LOGO
__TEXINFO_FLAGS += '--texinfo="@set LOGO $(_LOGO)"'
endif


# BUILD RULES:

ifdef _LOGO
# Prepare conversion of logo.
aux/$(_LOGO).svg: $(v)doc/$(_LOGO).svg
	@$(PRINTF_INFO) '\e[00;01;31mCP\e[34m %s\e[00m$A\n' "$@"
	@$(MKDIR) -p aux
	$(Q)$(CP) $^ $@ #$Z
	@$(ECHO_EMPTY)

# Intermediate format for the logo for DVI and PostScript manuals.
aux/$(_LOGO).ps: $(v)doc/$(_LOGO).svg
	@$(PRINTF_INFO) '\e[00;01;31mPS\e[34m %s\e[00m$A\n' "$@"
	@$(MKDIR) -p aux
	$(Q)$(SVG2PS) $^ > $@ #$Z
	@$(ECHO_EMPTY)

# Logo for DVI and PostScript manuals.
aux/$(_LOGO).eps: aux/$(_LOGO).ps
	@$(PRINTF_INFO) '\e[00;01;31mEPS\e[34m %s\e[00m$A\n' "$@"
	$(Q)$(PS2EPS) $^ #$Z
	@$(ECHO_EMPTY)

# Logo for PDF manual.
aux/$(_LOGO).pdf: $(v)doc/$(_LOGO).svg
	@$(PRINTF_INFO) '\e[00;01;31mPDF\e[34m %s\e[00m$A\n' "$@"
	@$(MKDIR) -p aux
	$(Q)$(SVG2PDF) $^ > $@ #$Z
	@$(ECHO_EMPTY)
endif

# Copy texinfo files to aux/doc.
aux/doc/%.texinfo: $(v)doc/info/%.texinfo
	@$(PRINTF_INFO) '\e[00;01;31mCP\e[34m %s\e[00m$A\n' "$@"
	@$(MKDIR) -p $(shell $(DIRNAME) $@)
	$(Q)$(CP) $< $@ #$Z
	@$(ECHO_EMPTY)

# Build info manual.
.PHONY: info
info: $(__TEXI_SRC) bin/$(_PROJECT).info
bin/%.info $(foreach P,$(__INFOPARTS),bin/%.info-$(P)): aux/doc/%.texinfo $(__TEXI_SRC)
	@$(PRINTF_INFO) '\e[00;01;31mTEXI\e[34m %s\e[00m$A\n' "$@"
	@$(MKDIR) -p bin
	$(Q)$(MAKEINFO) $(INFO_FLAGS) $< #$Z
	@$(PRINTF_INFO) '$A'
	$(Q)$(MV) $*.info $@ #$Z
	@$(ECHO_EMPTY)

# Build DVI manual.
.PHONY: dvi
dvi: $(__TEXI_SRC) bin/$(_PROJECT).dvi
bin/%.dvi: aux/doc/%.texinfo $(__TEXI_SRC) $(foreach L,$(_LOGO),aux/$(L).eps)
	@$(PRINTF_INFO) '\e[00;01;31mTEXI\e[34m %s\e[00m$A\n' "$@"
	@! $(TEST) -d aux/dvi/$* || $(RM) -rf aux/dvi/$*
	@$(MKDIR) -p aux/dvi/$* bin
	$(Q)cd aux/dvi/$* && $(TEXI2DVI) $(__back3unless_v)$< $(__TEXINFO_FLAGS) < /dev/null #$Z
	@$(PRINTF_INFO) '$A'
	$(Q)$(MV) aux/dvi/$*/$*.dvi $@ #$Z
	@$(ECHO_EMPTY)

# Build PDF manual.
.PHONY: pdf
pdf: $(__TEXI_SRC) bin/$(_PROJECT).pdf
bin/%.pdf: aux/doc/%.texinfo $(__TEXI_SRC) $(foreach L,$(_LOGO),aux/$(L).pdf)
	@$(PRINTF_INFO) '\e[00;01;31mTEXI\e[34m %s\e[00m$A\n' "$@"
	@! $(TEST) -d aux/pdf/$* || $(RM) -rf aux/pdf/$*
	@$(MKDIR) -p aux/pdf/$* bin
	$(Q)cd aux/pdf/$* && $(TEXI2PDF) $(__back3unless_v)$< $(__TEXINFO_FLAGS) < /dev/null #$Z
	@$(PRINTF_INFO) '$A'
	$(Q)$(MV) aux/pdf/$*/$*.pdf $@ #$Z
	@$(ECHO_EMPTY)

# Build PostScript manual.
.PHONY: ps
ps: $(__TEXI_SRC) bin/$(_PROJECT).ps
bin/%.ps: aux/doc/%.texinfo $(__TEXI_SRC) $(foreach L,$(_LOGO),aux/$(L).eps)
	@$(PRINTF_INFO) '\e[00;01;31mTEXI\e[34m %s\e[00m$A\n' "$@"
	@! $(TEST) -d aux/ps/$* || $(RM) -rf aux/ps/$*
	@$(MKDIR) -p aux/ps/$* bin
	$(Q)cd aux/ps/$* && $(TEXI2PS) $(__back3unless_v)$< $(__TEXINFO_FLAGS) < /dev/null #$Z
	@$(PRINTF_INFO) '$A'
	$(Q)$(MV) aux/ps/$*/$*.ps $@ #$Z
	@$(ECHO_EMPTY)

# Build HTML manual.
.PHONY: html
html: $(__TEXI_SRC) bin/html/$(_PROJECT)/index.html
bin/html/%/index.html: aux/doc/%.texinfo $(__TEXI_SRC)
	@$(PRINTF_INFO) '\e[00;01;31mTEXI\e[34m %s\e[00m$A\n' "$@"
	@! $(TEST) -d bin/html/$* || $(RM) -rf bin/html/$*
	@$(MKDIR) -p bin/html
	$(Q)cd bin/html && $(MAKEINFO_HTML) $(INFO_FLAGS) $(__back2unless_v)$< < /dev/null #$Z
	@$(ECHO_EMPTY)


# INSTALL RULES:

# Install info manual.
.PHONY: install-info
install-info: bin/$(_PROJECT).info $(foreach P,$(__INFOPARTS),bin/%.info-$(P))
	@$(PRINTF_INFO) '\e[00;01;31mINSTALL\e[34m %s\e[00m\n' "$@"
	$(Q)$(INSTALL_DIR) -- "$(DESTDIR)$(INFODIR)"
	$(Q)$(INSTALL_DATA) bin/$(_PROJECT).info -- "$(DESTDIR)$(INFODIR)/$(PKGNAME).info"
	$(Q)$(forearch P,$(__INFOPARTS),$(INSTALL_DATA) bin/$*.info-$(P) -- "$(DESTDIR)$(INFODIR)/$(PKGNAME).info-$(P)" &&) $(TRUE)
ifdef POST_INSTALL
	$(POST_INSTALL)
endif
	$(Q)if $(SHELL) -c '$(N) $(INSTALL_INFO) --version' > /dev/null 2>&1; then  \
	  $(N)$(z) $(INSTALL_INFO) -- "${DESTDIR}${INFODIR}/$(PKGNAME).info" "${DESTDIR}${INFODIR}/dir";  \
	else  \
	  $(TRUE);  \
	fi
ifdef POST_INSTALL
	$(NORMAL_INSTALL)
endif
	@$(ECHO_EMPTY)

# Install DVI manual.
.PHONY: install-dvi
install-dvi: bin/$(_PROJECT).dvi
	@$(PRINTF_INFO) '\e[00;01;31mINSTALL\e[34m %s\e[00m\n' "$@"
	$(Q)$(INSTALL_DIR) -- "$(DESTDIR)$(DVIDIR)"
	$(Q)$(INSTALL_DATA) $^ -- "$(DESTDIR)$(DVIDIR)/$(PKGNAME).dvi"
	@$(ECHO_EMPTY)

# Install PDF manual.
.PHONY: install-pdf
install-pdf: bin/$(_PROJECT).pdf
	@$(PRINTF_INFO) '\e[00;01;31mINSTALL\e[34m %s\e[00m\n' "$@"
	$(Q)$(INSTALL_DIR) -- "$(DESTDIR)$(PDFDIR)"
	$(Q)$(INSTALL_DATA) $^ -- "$(DESTDIR)$(PDFDIR)/$(PKGNAME).pdf"
	@$(ECHO_EMPTY)

# Install PostScript manual.
.PHONY: install-ps
install-ps: bin/$(_PROJECT).ps
	@$(PRINTF_INFO) '\e[00;01;31mINSTALL\e[34m %s\e[00m\n' "$@"
	$(Q)$(INSTALL_DIR) -- "$(DESTDIR)$(PSDIR)"
	$(Q)$(INSTALL_DATA) $^ -- "$(DESTDIR)$(PSDIR)/$(PKGNAME).ps"
	@$(ECHO_EMPTY)

# Install HTML manual.
.PHONY: install-html
install-html: $(foreach F,$(_HTML_FILES),bin/html/$(_PROJECT)/$(F))
	@$(PRINTF_INFO) '\e[00;01;31mINSTALL\e[34m %s\e[00m\n' "$@"
	$(Q)$(INSTALL_DIR) -- "$(DESTDIR)$(HTMLDIR)/$(PKGNAME)"
	$(Q)$(INSTALL_DATA) $^ -- "$(DESTDIR)$(HTMLDIR)/$(PKGNAME)/"
	@$(ECHO_EMPTY)


# UNINSTALL RULES:

# Uninstall info manual.
.PHONY: uninstall-info
uninstall-info:
ifdef PRE_UNINSTALL
	$(PRE_UNINSTALL)
endif
	-$(Q)$(N)$(a) $(INSTALL_INFO) --delete -- "${DESTDIR}${INFODIR}/$(PKGNAME).info" "${DESTDIR}${INFODIR}/dir"
ifdef PRE_UNINSTALL
	$(NORMAL_UNINSTALL)
endif
	-$(Q)$(RM) -- "$(DESTDIR)$(INFODIR)/$(PKGNAME).info" $(forearch P,$(__INFOPARTS),"$(DESTDIR)$(INFODIR)/$(PKGNAME).info-$(P)")

# Uninstall DVI manual.
.PHONY: uninstall-dvi
uninstall-dvi:
	-$(Q)$(RM) -- "$(DESTDIR)$(DVIDIR)/$(PKGNAME).dvi"

# Uninstall PDF manual.
.PHONY: uninstall-pdf
uninstall-pdf:
	-$(Q)$(RM) -- "$(DESTDIR)$(PDFDIR)/$(PKGNAME).pdf"

# Uninstall PostScript manual.
.PHONY: uninstall-ps
uninstall-ps:
	-$(Q)$(RM) -- "$(DESTDIR)$(PSDIR)/$(PKGNAME).ps"

# Uninstall HTML manual.
.PHONY: uninstall-html
uninstall-html:
	-$(Q)$(RM) -- $(foreach F,$(_HTML_FILES),"$(DESTDIR)$(HTMLDIR)/$(PKGNAME)/$(F)")
	-$(Q)$(RM) -- "$(DESTDIR)$(HTMLDIR)/$(PKGNAME)"


endif

