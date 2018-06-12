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
	TODO\
	THANKS\
	MAINTAINERS\
	src/lingot-logo.xpm

EXTRA_DIST = $(lingotdoc_DATA)

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

dist_man_MANS := lingot.1

# Copy all the spec files. Of course, only one is actually used.
dist-hook:
	$(MAKE) -C "$(srcdir)/po" "$(GETTEXT_PACKAGE).pot"
	cp "$(srcdir)/po/$(GETTEXT_PACKAGE).pot" "$(distdir)/po/"
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done
