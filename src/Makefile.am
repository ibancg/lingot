AM_CFLAGS = \
         -Wall -Wextra -pedantic \
	$(PACKAGE_CFLAGS) $(GTK_CFLAGS) $(ALSA_CFLAGS) $(JACK_CFLAGS) \
	-DLINGOT_LOCALEDIR=\""$(datadir)/locale"\"

bin_PROGRAMS = lingot

lingot_SOURCES = \
        lingot-gui-config-dialog.c\
	lingot-gui-config-dialog.h\
	lingot-gui-config-dialog.glade\
	lingot-gui-config-dialog-scale.c\
	lingot-gui-config-dialog-scale.h\
	lingot-gui-mainframe.c\
	lingot-gui-mainframe.h\
	lingot-gui-mainframe.glade\
        lingot.gresource.xml

nodist_lingot_SOURCES = lingot-resources.c
BUILT_SOURCES = lingot-resources.c

CLEANFILES = lingot-resources.c
		
lingot-resources.c: lingot.gresource.xml
	$(GLIB_COMPILE_RESOURCES) ${srcdir}/lingot.gresource.xml --target=$@ --sourcedir=${srcdir} --generate-source
        
lingot_LDFLAGS =

lib_LTLIBRARIES = liblingot.la
liblingot_la_SOURCES = \
    lingot-fft.c\
    lingot-fft.h\
    lingot-audio.c\
    lingot-audio.h\
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
    lingot-i18n.h
liblingot_la_LDFLAGS = -version-info 0:0:0 $(LIBFFTW_LIBS)

if OSS
    lib_LTLIBRARIES += liblingot-oss.la
    liblingot_oss_la_SOURCES = lingot-audio-oss.c\
                                lingot-audio-oss.h

    liblingot_oss_la_LDFLAGS = -version-info 0:0:0
endif

if ALSA
    lib_LTLIBRARIES += liblingot-alsa.la
    liblingot_alsa_la_SOURCES = lingot-audio-alsa.c\
                                lingot-audio-alsa.h

    liblingot_alsa_la_LDFLAGS = -version-info 0:0:0 $(ALSA_LIBS)
endif

if JACK
    lib_LTLIBRARIES += liblingot-jack.la
    liblingot_jack_la_SOURCES = lingot-audio-jack.c\
                                lingot-audio-jack.h

    liblingot_jack_la_LDFLAGS = -version-info 0:0:0 $(JACK_LIBS)
endif

if PULSEAUDIO
    lib_LTLIBRARIES += liblingot-pulseaudio.la
    liblingot_pulseaudio_la_SOURCES = lingot-audio-pulseaudio.c\
                                      lingot-audio-pulseaudio.h

    liblingot_pulseaudio_la_LDFLAGS = -version-info 0:0:0 $(PULSEAUDIO_LIBS)
endif

lingot_LDADD =  \
        $(PACKAGE_LIBS) $(GTK_LIBS) $(GLADE_LIBS) \
         -lpthread -ldl liblingot.la

