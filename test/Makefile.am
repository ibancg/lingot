## Process this file with automake to produce Makefile.in

if CUNIT
TESTS = lingot_test
check_PROGRAMS = lingot_test

# The -pthread compiler and linker option links with libpthread, but
# also selects various architecture-dependent settings.
lingot_test_LDFLAGS = \
  -pthread
lingot_test_CFLAGS = \
  -pthread \
  -Wall -Wextra -pedantic \
  -I${top_srcdir}/src \
  $(ALSA_CFLAGS) $(CUNIT_CFLAGS) $(JACK_CFLAGS) $(LIBFFTW_CFLAGS) $(OSS_CFLAGS) $(PULSEAUDIO_CFLAGS)
lingot_test_LDADD =  \
  $(ALSA_LIBS)   $(CUNIT_LIBS)   $(JACK_LIBS)   $(LIBFFTW_LIBS)   $(OSS_LIBS)   $(PULSEAUDIO_LIBS)

AUTOMAKE_OPTIONS = subdir-objects

lingot_test_SOURCES = \
	src/lingot-test.c \
	src/lingot-test.h \
	src/lingot-test-main.c \
	src/lingot-test-config-scale.c \
	src/lingot-test-core.c \
	src/lingot-test-io-config.c \
	src/lingot-test-signal.c
	
check_datadir =
check_data_DATA = resources/lingot-001.conf resources/lingot-0_9_2b8.conf resources/lingot-1_0_2b.conf

EXTRA_DIST = \
	$(check_data_DATA)

endif
