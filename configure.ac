dnl Process this file with autoconf to produce a configure script.

AC_INIT([Lingot],
        [1.1.0],
        [Iban Cereijo <ibancg@gmail.com>],
        [],
        [http://lingot.nongnu.org])
dnl useful for autoscan
AC_PREREQ([2.69])
AC_CONFIG_SRCDIR([src/lingot.c])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall subdir-objects])

AC_CONFIG_HEADERS([config.h])

AM_PROG_AR
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_INTLTOOL([0.23])
AC_PROG_MKDIR_P

LT_INIT

AC_PATH_PROG([GLIB_COMPILE_RESOURCES],[glib-compile-resources])
PKG_PROG_PKG_CONFIG([0.22])
GLIB_GSETTINGS

dnl Switching to autopoint causes a warning about AC_PROG_MKDIR_P
dnl being preferred.  I do not know why, but at least the culprint is
dnl not configure.ac.
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19])

dnl For src/lingot.c
AC_DEFINE_UNQUOTED([LOCALEDIR], ["$localedir"],
  [directory where translations are installed])

dnl For po/Makefile.in.in
AC_SUBST([GETTEXT_PACKAGE], [lingot])

AC_CHECK_LIB(m, sin)

dnl This list can be checked with autoscan
AC_CHECK_HEADERS([langinfo.h])
AC_CHECK_HEADERS([libintl.h])
AC_CHECK_HEADERS([locale.h])
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([complex.h])
AC_CHECK_FUNCS([floor])
AC_CHECK_FUNCS([gettimeofday])
AC_CHECK_FUNCS([memmove])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([pow])
AC_CHECK_FUNCS([rint])
AC_CHECK_FUNCS([setlocale])
AC_CHECK_FUNCS([sqrt])
AC_CHECK_FUNCS([strchr])
AC_CHECK_FUNCS([strdup])
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([strrchr])
AC_FUNC_MALLOC
AC_FUNC_STRTOD
AC_TYPE_INT16_T
AC_TYPE_SIZE_T

PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= 3.10])
PKG_CHECK_MODULES([JSON], [json-c >= 0.10.0])

AC_ARG_WITH([alsa],
  [AS_HELP_STRING([--with-alsa], [enable ALSA sound @<:@default=check@:>@])],
  [], [with_alsa=check])
AS_IF([test "x$with_alsa" != xno],
  [PKG_CHECK_MODULES([ALSA], [alsa],
    [AC_DEFINE([ALSA], [1], [Define to enable ALSA sound])],
    [if test "x$with_alsa" != xcheck; then
      AC_MSG_FAILURE([--with-alsa was given, but pkg-config does not find libalsa])
     fi])])

AC_ARG_WITH([cunit],
  [AS_HELP_STRING([--with-cunit], [enable CUnit tests @<:@default=check@:>@])],
  [], [with_cunit=check])
AS_IF([test "x$with_cunit" != xno],
  [PKG_CHECK_MODULES([CUNIT], [cunit],
    [cunit_found=true],
    [if test "x$with_cunit" != xcheck; then
      AC_MSG_FAILURE([--with-cunit was given, but pkg-config does not find libcunit])
     fi])])
AM_CONDITIONAL([CUNIT], [test "x$cunit_found" = xtrue])

AC_ARG_WITH([fftw],
  [AS_HELP_STRING([--with-fftw], [enable fftw Fourier transform @<:@default=check@:>@])],
  [], [with_fftw=check])
AS_IF([test "x$with_fftw" != xno],
  [PKG_CHECK_MODULES([LIBFFTW], [fftw3 >= 3.2.2 fftw3f >= 3.2.2],
    [AC_DEFINE([LIBFFTW], [1], [Define to enable fftw Fourier transform])],
    [if test "x$with_fftw" != xcheck; then
       AC_MSG_FAILURE([--with-fftw was given, but pkg-config does not find libfftw3 or libfftw3f])
     else
       AC_MSG_WARN([No libfftw was found : FFT will be slower])
     fi])])

AC_ARG_WITH([jack],
  [AS_HELP_STRING([--with-jack], [enable JACK sound @<:@default=check@:>@])],
  [], [with_jack=check])
AS_IF([test "x$with_jack" != xno],
  [PKG_CHECK_MODULES([JACK],
    [jack >= 0.117.0],
    [AC_DEFINE([JACK], [1], [Define to enable JACK sound])],
    [if test "x$with_jack" != xcheck; then
      AC_MSG_FAILURE([--with-jack was given, but pkg-config does not find libjack])
     fi])])

AC_ARG_WITH([oss],
  [AS_HELP_STRING([--with-oss], [enable OSS sound @<:@default=check@:>@])],
  [], [with_oss=yes])
AS_IF([test "x$with_oss" != xno],
  [AC_DEFINE([OSS], [1], [Define to enable OSS sound])
   AC_CHECK_HEADERS([fcntl.h])
   AC_CHECK_HEADERS([sys/ioctl.h])])

AC_ARG_WITH([pulseaudio],
  [AS_HELP_STRING([--with-pulseaudio], [enable PulseAudio sound @<:@default=check@:>@])],
  [], [with_pulseaudio=check])
AS_IF([test "x$with_pulseaudio" != xno],
  [PKG_CHECK_MODULES([PULSEAUDIO], [libpulse-simple >= 0.9.10],
    [AC_DEFINE([PULSEAUDIO], [1], [Define to enable PulseAudio sound])],
    [if test "x$with_pulseaudio" != xcheck; then
       AC_MSG_FAILURE([--with-pulseaudio was given, but pkg-config does not find libpulse-simple])
     fi])])

AC_CONFIG_FILES([
Makefile
po/Makefile.in
])
AC_OUTPUT
