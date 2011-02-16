SUBDIRS = glade

INCLUDES = \
	$(PACKAGE_CFLAGS) $(GTK_CFLAGS) $(ALSA_CFLAGS) $(JACK_CFLAGS) $(GLADE_CFLAGS)\
	-DLINGOT_GLADEDIR=\""$(datadir)/lingot/glade/"\" \
	-DLINGOT_LOCALEDIR=\""$(datadir)/locale"\"

AM_CXXFLAGS =\
	 -Wall

bin_PROGRAMS = lingot

lingot_SOURCES = \
	lingot-fft.c\
	lingot-fft.h\
	lingot-audio.c\
	lingot-audio.h\
	lingot-audio-oss.c\
	lingot-audio-oss.h\
	lingot-audio-alsa.c\
	lingot-audio-alsa.h\
	lingot-audio-jack.c\
	lingot-audio-jack.h\
	lingot-complex.h\
	lingot-complex.c\
	lingot-config.c\
	lingot-config.h\
	lingot-core.c\
	lingot-core.h\
	lingot-defs.h\
	lingot-error.h\
	lingot-error.c\
	lingot-config-dialog.c\
	lingot-config-dialog.h\
	lingot-config-dialog-scale.c\
	lingot-config-dialog-scale.h\
	lingot-gauge.c\
	lingot-gauge.h\
	lingot-mainframe.c\
	lingot-mainframe.h\
	lingot-filter.c\
	lingot-filter.h\
	lingot-signal.c\
	lingot-signal.h\
	lingot.c\
	lingot-i18n.h

lingot_LDFLAGS =

lingot_LDADD =  \
	$(PACKAGE_LIBS) $(GTK_LIBS) $(ALSA_LIBS) $(JACK_LIBS) $(GLADE_LIBS)\
	 -lpthread

