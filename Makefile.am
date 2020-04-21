## Process this file with automake to produce Makefile.in

# Unless otherwise specified, the numbers refer to
# https://www.gnu.org/software/automake/manual/automake.html

######################################################################
# https://www.gnu.org/software/gettext/manual/gettext.html
# 13.4.8 aclocal.m4 at top level

ACLOCAL_AMFLAGS = -I m4

######################################################################
# 7 directories

# Generally avoid recursive make as explained in 7.3, but gettext
# insists on a po/ subdirectory.
SUBDIRS = po

######################################################################
# 8 building programs and libraries

# The -pthread compiler and linker option links with libpthread, but
# also selects various architecture-dependent settings.
AM_LDFLAGS = -pthread -Wl,--as-needed
AM_CFLAGS = -pthread -Wall -Wextra -pedantic

bin_PROGRAMS = lingot

lib_LTLIBRARIES = liblingot.la
liblingot_la_SOURCES = \
    src/lingot-audio-alsa.c \
    src/lingot-audio-jack.c \
    src/lingot-audio-oss.c \
    src/lingot-audio-pulseaudio.c \
    src/lingot-audio.c \
    src/lingot-complex.c \
    src/lingot-complex.h \
    src/lingot-config-scale.c \
    src/lingot-config.c \
    src/lingot-core.c \
    src/lingot-defs.c \
    src/lingot-defs.h \
    src/lingot-fft.c \
    src/lingot-fft.h \
    src/lingot-filter.c \
    src/lingot-gauge.c \
    src/lingot-gauge.h \
    src/lingot-i18n.h \
    src/lingot-io-config-scale.c \
    src/lingot-io-config.c \
    src/lingot-msg.c \
    src/lingot-signal.c \
    src/lingot-signal.h

pkginclude_HEADERS = \
    src/lingot-audio-alsa.h \
    src/lingot-audio-jack.h \
    src/lingot-audio-oss.h \
    src/lingot-audio-pulseaudio.h \
    src/lingot-audio.h \
    src/lingot-config-scale.h \
    src/lingot-config.h \
    src/lingot-core.h \
    src/lingot-filter.h \
    src/lingot-io-config-scale.h \
    src/lingot-io-config.h \
    src/lingot-msg.h

src/lingot-resources.c: src/lingot.gresource.xml
	$(GLIB_COMPILE_RESOURCES) $< --target=$@ --sourcedir=$(srcdir)/src --generate-source

# libtool 7.3 updating library version information
# Downstream packagers may have to override the SO version.
lingot_shared_object_version = 0:0:0

# Options to build the library itself (dependencies)
liblingot_dep_cflags = $(ALSA_CFLAGS) $(JACK_CFLAGS) $(LIBFFTW_CFLAGS) $(OSS_CFLAGS) $(PULSEAUDIO_CFLAGS)
liblingot_dep_libs   = $(ALSA_LIBS)   $(JACK_LIBS)   $(LIBFFTW_LIBS)   $(OSS_LIBS)   $(PULSEAUDIO_LIBS)

# Locally link against the library.
liblingot_rdep_cflags = -I$(srcdir)/src $(liblingot_dep_cflags)
liblingot_rdep_libs   = liblingot.la    $(liblingot_dep_libs)

liblingot_la_CFLAGS = $(AM_CFLAGS) $(liblingot_dep_cflags)
liblingot_la_LIBADD =              $(liblingot_dep_libs)

liblingot_la_LDFLAGS = $(AM_LDFLAGS) \
  -version-info $(lingot_shared_object_version) \
  -Wl,--no-allow-shlib-undefined \
  -Wl,--no-copy-dt-needed-entries \
  -Wl,--no-undefined

lingot_SOURCES = \
    src/lingot-gui-config-dialog-scale.c \
    src/lingot-gui-config-dialog-scale.h \
    src/lingot-gui-config-dialog.c \
    src/lingot-gui-config-dialog.glade \
    src/lingot-gui-config-dialog.h \
    src/lingot-gui-gauge.c \
    src/lingot-gui-gauge.h \
    src/lingot-gui-i18n.h \
    src/lingot-gui-mainframe.c \
    src/lingot-gui-mainframe.glade \
    src/lingot-gui-mainframe.h \
    src/lingot-gui-spectrum.c \
    src/lingot-gui-spectrum.h \
    src/lingot.c \
    src/lingot.gresource.xml

nodist_lingot_SOURCES = src/lingot-resources.c

lingot_CFLAGS = $(AM_CFLAGS) $(liblingot_rdep_cflags) $(GTK_CFLAGS)
lingot_LDADD  =              $(liblingot_rdep_libs)   $(GTK_LIBS)

######################################################################
# 11 building documentation

dist_man_MANS = lingot.1

######################################################################
# 12 what gets installed

dist_doc_DATA = \
  AUTHORS \
  COPYING \
  ChangeLog \
  NEWS \
  README \
  README.config \
  README.md \
  THANKS

iconsdir = $(datadir)/icons/hicolor/scalable/apps
dist_icons_DATA = icons/org.nongnu.lingot.svg

gnomemenudir = $(datadir)/applications
dist_gnomemenu_DATA = org.nongnu.lingot.desktop

appdatadir = $(datadir)/metainfo
dist_appdata_DATA = org.nongnu.lingot.appdata.xml

# Install to architecture-dependent directory. Do not distribute.
pkgexecconfigdir = $(libdir)/pkgconfig
pkgexecconfig_DATA = lingot.pc

# autoconf 4.8.2 Installation Directory Variables
# Substituting from here instead of from configure.ac ensures that the
# user can override prefix on the Make command line.
edit = sed \
  -e 's|@PACKAGE_URL[@]|$(PACKAGE_URL)|g' \
  -e 's|@PACKAGE_VERSION[@]|$(PACKAGE_VERSION)|g' \
  -e 's|@OSS_CFLAGS[@]|$(OSS_CFLAGS)|g' \
  -e 's|@pkgincludedir[@]|$(pkgincludedir)|g' \
  -e 's|@libdir[@]|$(libdir)|g' \
  -e 's|@OSS_LIBS[@]|$(OSS_LIBS)|g'
lingot.pc: Makefile
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	mv $@.tmp $@
lingot.pc: $(srcdir)/lingot.pc.in

######################################################################
# 13 what gets cleaned

CLEANFILES = \
  lingot.pc \
  src/lingot-resources.c

MAINTAINERCLEANFILES = \
  ABOUT-NLS \
  INSTALL \
  Makefile.in \
  aclocal.m4 \
  ar-lib \
  compile \
  config.guess \
  config.h.in \
  config.rpath \
  config.sub \
  configure \
  depcomp \
  install-sh \
  ltmain.sh \
  m4/* \
  missing \
  po/*.header \
  po/*.sed \
  po/*.sin \
  po/Makevars.template \
  po/Rules-quot \
  test-driver

######################################################################
# 14 what goes in a distribution

# bootstrap and tests require additional dependencies but may be
# useful to users of the tarball
EXTRA_DIST = \
  bootstrap \
  lingot.pc.in \
  test/resources/lingot-001.conf \
  test/resources/lingot-0_9_2b8.conf \
  test/resources/lingot-1_0_2b.conf

######################################################################
# 15 support for test suites

# And also 20 conditionals, because we only want to build and run
# tests if CUnit is available.
# EXTRA_PROGRAMS does not look easyer...
# Automake accepts no indentation here.

# Help the executable finding the shared library.
AM_TESTS_ENVIRONMENT = export LD_LIBRARY_PATH='$(builddir)/.libs';

if CUNIT
# Programs run by 'make check'.
TESTS = lingot_test

# Programs built for 'make check'.
check_PROGRAMS = lingot_test

lingot_test_SOURCES = \
  test/src/lingot-test-config-scale.c \
  test/src/lingot-test-core.c \
  test/src/lingot-test-io-config.c \
  test/src/lingot-test-main.c \
  test/src/lingot-test-signal.c \
  test/src/lingot-test.c \
  test/src/lingot-test.h

lingot_test_CFLAGS = $(AM_CFLAGS) $(liblingot_rdep_cflags) $(CUNIT_CFLAGS)
lingot_test_LDADD  =              $(liblingot_rdep_libs)   $(CUNIT_LIBS)

endif
