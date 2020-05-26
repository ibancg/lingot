/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2020  Iban Cereijo.
 * Copyright (C) 2004-2008  Jairo Chapela.

 *
 * This file is part of lingot.
 *
 * lingot is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * lingot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with lingot; if not, write to the Free Software Foundation,
 * Inc. 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef LINGOT_INTERNAL_DEFS_H
#define LINGOT_INTERNAL_DEFS_H

#include "lingot-defs.h"

#ifdef __cplusplus
extern "C" {
#endif

// simple try-catch simulation, do not use throw inside loops nor nest try-catch
// blocks
#define _try _exception = 0; do
#define _throw(a) { _exception = a;break; }
#define _catch while (0); if (_exception != 0)

// This alternative allows us to throw exception from loops, it contains a goto
// statement, but totally controlled. It fails when trying to indent code.
//#define _try _exception = 0;do
//#define _throw(a) {_exception = a;goto catch_label;}
//#define _catch while (0);catch_label: if (_exception != 0)

#ifndef M_PI
#    define M_PI 3.14159265358979323846
#endif

// strdup() is not part of the standard. It is POSIX, but we provide our own
// implementation.
char* _strdup(const char *s);

#ifdef __cplusplus
}
#endif

#endif
