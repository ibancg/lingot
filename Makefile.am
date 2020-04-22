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
AM_LDFLAGS = -pthread
AM_CFLAGS = -pthread -Wall -Wextra -pedantic

bin_PROGRAMS = lingot

lingot_SOURCES = \
  src/lingot-audio-alsa.c \
  src/lingot-audio-alsa.h \
  src/lingot-audio-jack.c \
  src/lingot-audio-jack.h \
  src/lingot-audio-oss.c \
  src/lingot-audio-oss.h \
  src/lingot-audio-pulseaudio.c \
  src/lingot-audio-pulseaudio.h \
  src/lingot-audio.c \
  src/lingot-audio.h \
  src/lingot-complex.c \
  src/lingot-complex.h \
  src/lingot-config-scale.c \
  src/lingot-config-scale.h \
  src/lingot-config.c \
  src/lingot-config.h \
  src/lingot-core.c \
  src/lingot-core.h \
  src/lingot-defs.c \
  src/lingot-defs.h \
  src/lingot-fft.c \
  src/lingot-fft.h \
  src/lingot-filter.c \
  src/lingot-filter.h \
  src/lingot-gui-config-dialog-scale.c \
  src/lingot-gui-config-dialog-scale.h \
  src/lingot-gui-config-dialog.c \
  src/lingot-gui-config-dialog.glade \
  src/lingot-gui-config-dialog.h \
  src/lingot-gui-gauge.c \
  src/lingot-gui-gauge.h \
  src/lingot-gui-mainframe.c \
  src/lingot-gui-mainframe.glade \
  src/lingot-gui-mainframe.h \
  src/lingot-gui-spectrum.c \
  src/lingot-gui-spectrum.h \
  src/lingot-i18n.h \
  src/lingot-io-config-scale.c \
  src/lingot-io-config-scale.h \
  src/lingot-io-config.c \
  src/lingot-io-config.h \
  src/lingot-msg.c \
  src/lingot-msg.h \
  src/lingot-signal.c \
  src/lingot-signal.h \
  src/lingot.c \
  src/lingot.gresource.xml

lingot_CPPFLAGS = \
  $(ALSA_CFLAGS) $(GTK_CFLAGS) $(JACK_CFLAGS) $(LIBFFTW_CFLAGS) $(OSS_CFLAGS) $(PULSEAUDIO_CFLAGS)
lingot_LDADD = \
  $(ALSA_LIBS)   $(GTK_LIBS)   $(JACK_LIBS)   $(LIBFFTW_LIBS)   $(OSS_LIBS)   $(PULSEAUDIO_LIBS)

######################################################################
# 9 other derived objects

nodist_lingot_SOURCES = src/lingot-resources.c
BUILT_SOURCES = src/lingot-resources.c
src/lingot-resources.c: src/lingot.gresource.xml
	$(GLIB_COMPILE_RESOURCES) $< --target=$@ --sourcedir=$(srcdir)/src --generate-source

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

######################################################################
# 13 what gets cleaned

CLEANFILES = $(BUILT_SOURCES)

MAINTAINERCLEANFILES = \
  ABOUT-NLS \
  config.rpath \
  m4/* \
  po/*.header \
  po/*.sed \
  po/*.sin \
  po/Makevars.template \
  po/Rules-quot

######################################################################
# 14 what goes in a distribution

# bootstrap and tests require additional dependencies but may be
# useful to users of the tarball
EXTRA_DIST = \
  bootstrap \
  test/resources/lingot-001.conf \
  test/resources/lingot-0_9_2b8.conf \
  test/resources/lingot-1_0_2b.conf

######################################################################
# 15 support for test suites

# And also 20 conditionals, because we only want to build and run
# tests if CUnit is available.
# EXTRA_PROGRAMS does not look easyer...
# Automake accepts no indentation here.

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

lingot_test_CPPFLAGS = \
  -I$(srcdir)/src \
  $(ALSA_CFLAGS) $(CUNIT_CFLAGS) $(JACK_CFLAGS) $(LIBFFTW_CFLAGS) $(OSS_CFLAGS) $(PULSEAUDIO_CFLAGS)
lingot_test_LDADD = \
  $(ALSA_LIBS)   $(CUNIT_LIBS)   $(JACK_LIBS)   $(LIBFFTW_LIBS)   $(OSS_LIBS)   $(PULSEAUDIO_LIBS)

endif
