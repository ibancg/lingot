## Process this file with automake to produce Makefile.in
## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

SUBDIRS = po src pixmaps

ACLOCAL_AMFLAGS = -I m4

lingotdocdir = ${docdir}
lingotdoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	NEWS\
	THANKS

DISTCLEANFILES = \
	intltool-extract.in\
	intltool-extract\
	intltool-merge.in\
	intltool-merge\
	intltool-update.in\
	intltool-update\
	intl/*\
	m4/*\
	po/Makefile.in.in

gnomemenudir = $(prefix)/@NO_PREFIX_PACKAGE_MENU_DIR@
gnomemenu_DATA = lingot.desktop
appdatadir = $(datadir)/metainfo
appdata_DATA = lingot.appdata.xml

dist_man_MANS = lingot.1

EXTRA_DIST = $(lingotdoc_DATA) $(appdata_DATA)

dist-hook: gen-ChangeLog copy-SpecFiles

# Creates the ChangeLog from the git history. Extracted from GNU Hello
.PHONY: gen-ChangeLog
gen-ChangeLog:
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

	