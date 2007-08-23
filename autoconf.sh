#!/bin/sh
# Script provisional para crear los makefiles iniciales, etc ...
# Este fichero no se distribuye
# Ibán

aclocal
autoheader
glib-gettextize --copy --force
intltoolize --copy --force
autoconf
automake --add-missing
