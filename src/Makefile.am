AM_CFLAGS = \
         -Wall -Wextra -pedantic \
	$(PACKAGE_CFLAGS) $(GTK_CFLAGS) $(ALSA_CFLAGS) $(JACK_CFLAGS) \
	-DLINGOT_LOCALEDIR=\""$(datadir)/locale"\"

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
	lingot-audio-pulseaudio.c\
	lingot-audio-pulseaudio.h\
	lingot-complex.h\
	lingot-complex.c\
	lingot-config.c\
	lingot-config.h\
	lingot-config-scale.c\
	lingot-config-scale.h\
	lingot-core.c\
	lingot-core.h\
	lingot-defs.h\
	lingot-msg.h\
	lingot-msg.c\
	lingot-gui-config-dialog.c\
	lingot-gui-config-dialog.h\
	lingot-gui-config-dialog.glade\
	lingot-gui-config-dialog-scale.c\
	lingot-gui-config-dialog-scale.h\
	lingot-gui-mainframe.c\
	lingot-gui-mainframe.h\
	lingot-gui-mainframe.glade\
	lingot-gauge.c\
	lingot-gauge.h\
	lingot-filter.c\
	lingot-filter.h\
	lingot-io-config.c\
	lingot-io-config.h\
        lingot-io-config-scale.c\
        lingot-io-config-scale.h\
        lingot-signal.c\
	lingot-signal.h\
	lingot.c\
	lingot-i18n.h\
	lingot.gresource.xml

nodist_lingot_SOURCES = lingot-resources.c
BUILT_SOURCES = lingot-resources.c

CLEANFILES = lingot-resources.c
		
lingot-resources.c: lingot.gresource.xml
	$(GLIB_COMPILE_RESOURCES) ${srcdir}/lingot.gresource.xml --target=$@ --sourcedir=${srcdir} --generate-source
        
lingot_LDFLAGS =

lingot_LDADD =  \
	$(PACKAGE_LIBS) $(GTK_LIBS) $(ALSA_LIBS) $(JACK_LIBS) $(PULSEAUDIO_LIBS) $(GLADE_LIBS) $(LIBFFTW_LIBS) \
	 -lpthread

