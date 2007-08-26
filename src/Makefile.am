## Process this file with automake to produce Makefile.in

## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

INCLUDES = \
	$(PACKAGE_CFLAGS)\
	-DLINGOT_LOCALEDIR=\""$(datadir)/locale"\"

AM_CXXFLAGS =\
	 -Wall

bin_PROGRAMS = lingot

lingot_SOURCES = \
	lingot-fft.c\
	lingot-fft.h\
	lingot-audio.c\
	lingot-audio.h\
	lingot-complex.h\
	lingot-complex.c\
	lingot-config.c\
	lingot-config.h\
	lingot-core.c\
	lingot-core.h\
	lingot-defs.h\
	lingot-config-dialog.c\
	lingot-config-dialog.h\
	lingot-filters.h\
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
	$(PACKAGE_LIBS)\
	 -lpthread

