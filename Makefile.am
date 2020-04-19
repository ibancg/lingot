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
	bootstrap

dist-hook:
	$(MAKE) -C "$(srcdir)/po" "$(GETTEXT_PACKAGE).pot"
	cp "$(srcdir)/po/$(GETTEXT_PACKAGE).pot" "$(distdir)/po/"
