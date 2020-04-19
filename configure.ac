dnl Process this file with autoconf to produce a configure script.

AC_INIT([Lingot], [1.0.2b])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE
LT_INIT

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CC_STDC
AC_HEADER_STDC

AC_PROG_INTLTOOL([0.23])

AC_PATH_PROG([GLIB_COMPILE_RESOURCES],[glib-compile-resources])
PKG_PROG_PKG_CONFIG([0.22])
GLIB_GSETTINGS

dnl Languages which your application supports
ALL_LINGUAS="ca cs de es gl it pt_BR sv zh_CN fr"
AM_GLIB_GNU_GETTEXT

AC_DEFINE_UNQUOTED([LOCALEDIR], ["$localedir"],
  [directory where translations are installed])

AC_PROG_LIBTOOL

AC_CHECK_LIB(m, sin)
AC_CHECK_HEADERS([langinfo.h])
AC_CHECK_HEADERS([libintl.h])
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_FUNCS([floor])
AC_CHECK_FUNCS([gettimeofday])
AC_CHECK_FUNCS([memmove])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([mkdir])
AC_CHECK_FUNCS([pow])
AC_CHECK_FUNCS([rint])
AC_CHECK_FUNCS([setlocale])
AC_CHECK_FUNCS([sqrt])
AC_CHECK_FUNCS([strchr])
AC_CHECK_FUNCS([strdup])
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([strrchr])
AC_CHECK_FUNCS([strcasecmp])
AC_FUNC_MALLOC

PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= 3.10])
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

AC_ARG_ENABLE(
  oss,
  AC_HELP_STRING([--enable-oss], [use OSS @<:@default=yes@:>@]),
  [], [enable_oss=yes])

AM_CONDITIONAL([OSS], [test "x$enable_oss" = "xyes"])
AM_COND_IF([OSS],
    [AC_DEFINE(OSS, 1, [Define to 0 for Something else])],[])

if test "x$enable_oss" = "xyes"; then
	AC_CHECK_HEADERS([fcntl.h])
	AC_CHECK_HEADERS([sys/ioctl.h])
	AC_SUBST(OSS_CFLAGS)
	AC_SUBST(OSS_LIBS)
        CFLAGS="$CFLAGS -DOSS"
fi

AC_ARG_ENABLE(
  alsa,
  AC_HELP_STRING([--enable-alsa], [use ALSA @<:@default=yes@:>@]),
  [], [enable_alsa=yes])

AM_CONDITIONAL([ALSA], [test "x$enable_alsa" = "xyes"])
AM_COND_IF([ALSA],
    [AC_DEFINE(ALSA, 1, [Define to 0 for Something else])],[])

if test "x$enable_alsa" = "xyes"; then
	PKG_CHECK_MODULES([ALSA], [alsa])
	AC_SUBST(ALSA_CFLAGS)
	AC_SUBST(ALSA_LIBS)
        CFLAGS="$CFLAGS -DALSA"
fi

AC_ARG_ENABLE(
  jack,
  AC_HELP_STRING([--enable-jack], [use JACK @<:@default=yes@:>@]),
  [], [enable_jack=yes])

AM_CONDITIONAL(JACK, test "x$enable_jack" = "xyes")

if test "x$enable_jack" = "xyes"; then
        PKG_CHECK_MODULES(JACK, jack >= 0.117.0)
	AC_SUBST(JACK_CFLAGS)
	AC_SUBST(JACK_LIBS)
        CFLAGS="$CFLAGS -DJACK"
fi

AC_ARG_ENABLE(
  pulseaudio,
  AC_HELP_STRING([--enable-pulseaudio], [use PulseAudio @<:@default=yes@:>@]),
  [], [enable_pulseaudio=yes])

AM_CONDITIONAL(PULSEAUDIO, test "x$enable_pulseaudio" = "xyes")
AM_COND_IF([PULSEAUDIO],
    [AC_DEFINE(PULSEAUDIO, 1, [Define to 0 for Something else])],[])

if test "x$enable_pulseaudio" = "xyes"; then
 	PKG_CHECK_MODULES(PULSEAUDIO, libpulse-simple >= 0.9.10)
	AC_SUBST(PULSEAUDIO_CFLAGS)
	AC_SUBST(PULSEAUDIO_LIBS)
        CFLAGS="$CFLAGS -DPULSEAUDIO"
fi

uselibfftw=yes
fftw_found=no

AC_ARG_ENABLE(
  libfftw,
  AC_HELP_STRING([--enable-libfftw], [use libfftw @<:@default=yes@:>@]),
  [
    if test "x$enableval" = "xno"; then
      uselibfftw=no
    fi
  ])

dnl AM_CONDITIONAL(HAVE_LIBFFTW, test "x$uselibfftw" = "xyes")

if test "x$uselibfftw" = "xyes"; then
 PKG_CHECK_MODULES([LIBFFTW],
                    [fftw3 >= 3.2.2
                    fftw3f >= 3.2.2],
                    [fftw_found=yes],
                    [fftw_found=no])
 if test "x$fftw_found" = xyes ; then
	AC_SUBST([LIBFFTW_CFLAGS])
	AC_SUBST([LIBFFTW_LIBS])
	CFLAGS="$CFLAGS -DLIBFFTW"
 else
  AC_MSG_WARN([ No libfftw was found : FFT will be slower ])
 fi
fi

AC_CONFIG_FILES([
Makefile
po/Makefile.in
src/Makefile
icons/Makefile
test/Makefile
])
AC_OUTPUT
