## Process this file with automake to produce Makefile.in

SUBDIRS = po src icons test

ACLOCAL_AMFLAGS = -I m4

lingotdocdir = ${docdir}
lingotdoc_DATA = \
	README \
	COPYING \
	AUTHORS \
	ChangeLog \
	NEWS \
	THANKS

DISTCLEANFILES = \
	intltool-extract.in \
	intltool-extract \
	intltool-merge.in \
	intltool-merge \
	intltool-update.in \
	intltool-update \
	intl/* \
	m4/* \
	po/Makefile.in.in

gnomemenudir = $(datadir)/applications
gnomemenu_DATA = org.nongnu.lingot.desktop
appdatadir = $(datadir)/metainfo
appdata_DATA = org.nongnu.lingot.appdata.xml

dist_man_MANS = lingot.1


EXTRA_DIST = \
	$(lingotdoc_DATA) \
	$(gnomemenu_DATA) \
	$(appdata_DATA) \
	build-aux/gitlog-to-changelog \
	bootstrap

dist-hook: gen-ChangeLog copy-SpecFiles

# Creates the ChangeLog from the git history. Extracted from GNU Hello
.PHONY: gen-ChangeLog
gen-ChangeLog:
	chmod u+w ${distdir}/ChangeLog # to pass distcheck
	$(top_srcdir)/build-aux/gitlog-to-changelog > $(distdir)/ChangeLog
	
# Copy all the spec files. Of course, only one is actually used.
copy-SpecFiles:
	$(MAKE) -C "$(srcdir)/po" "$(GETTEXT_PACKAGE).pot"
	cp "$(srcdir)/po/$(GETTEXT_PACKAGE).pot" "$(distdir)/po/"
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done
