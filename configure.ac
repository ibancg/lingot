#  Copyright (C) 2004   Ibán Cereijo Graña, Jairo Chapela Martínez.
#
#  This file is part of lingot.
#
#  lingot is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#  
#  lingot is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  
#  You should have received a copy of the GNU General Public License
#  along with lingot; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#


# Process this file with autoconf to produce a configure script.
AC_INIT(lingot, 0.6.0, BUG-REPORT-ADDRESS)
AC_CONFIG_SRCDIR([src/config.cc])
#AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC


AC_PATH_PROG(GTK_CONFIG, gtk-config, no)
no_gtk=""
if test "$GTK_CONFIG" != "no" ; then
    GTK_CFLAGS=`$GTK_CONFIG $gtk_config_args --cflags`
    GTK_LIBS=`$GTK_CONFIG $gtk_config_args --libs`
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $GTK_CFLAGS"
    LIBS="$LIBS $GTK_LIBS"
fi

# Checks for libraries.
AC_CHECK_LIB(gtk, gtk_main, [], [AC_MSG_ERROR(GTK 1.2 or above not found!)])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h langinfo.h libintl.h locale.h memory.h stdlib.h string.h sys/ioctl.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_HEADER_TIME

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_CHECK_FUNCS([gettimeofday memset mkdir pow rint select setlocale sqrt])

AC_CONFIG_FILES([])
AC_OUTPUT([Makefile])
