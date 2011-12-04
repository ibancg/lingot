## Process this file with automake to produce Makefile.in
## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

SUBDIRS = po src pixmaps

lingotdocdir = ${docdir}
lingotdoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS\
	TODO\
	THANKS\
	MAINTAINERS\
	src/lingot-background.xpm\
	src/lingot-logo.xpm\
	intltool-extract.in\
	intltool-merge.in\
	intltool-update.in

EXTRA_DIST = $(lingotdoc_DATA)

CLEANFILES =\
	intltool-extract\
	intltool-merge\
	intltool-update

gnomemenudir = $(prefix)/@NO_PREFIX_PACKAGE_MENU_DIR@
gnomemenu_DATA = lingot.desktop



# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	$(MAKE) -C "$(srcdir)/po" "$(GETTEXT_PACKAGE).pot"
	cp "$(srcdir)/po/$(GETTEXT_PACKAGE).pot" "$(distdir)/po/"
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done
