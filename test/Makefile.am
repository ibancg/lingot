## Process this file with automake to produce Makefile.in
## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

TESTS = lingot_test
check_PROGRAMS = lingot_test

lingot_test_CFLAGS = \
	-I${top_srcdir}/src \
	-Wall -Wextra \
 	$(PACKAGE_CFLAGS) $(GTK_CFLAGS) $(ALSA_CFLAGS) $(JACK_CFLAGS)

lingot_test_LDFLAGS =

lingot_test_LDADD =  \
	-L/usr/local/include -lcunit \
	$(PACKAGE_LIBS) $(GTK_LIBS) $(ALSA_LIBS) $(JACK_LIBS) $(PULSEAUDIO_LIBS) $(GLADE_LIBS) $(LIBFFTW_LIBS) \
	 -lpthread
	
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
	
