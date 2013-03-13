/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2013  Ibán Cereijo Graña.
 * Copyright (C) 2004-2008  Jairo Chapela Martínez.

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
 * along with lingot; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __LINGOT_DEFS_H__
#define __LINGOT_DEFS_H__

#include "../config.h"

// floating point precission.
#define FLT                  double

#define CONFIG_DIR_NAME           ".lingot/"
#define DEFAULT_CONFIG_FILE_NAME  "lingot.conf"
extern char CONFIG_FILE_NAME[];

#define QUICK_MESSAGES

#define GTK_EVENTS_RATE      20.0
#define GAUGE_RATE           60.0
#define ERROR_DISPATCH_RATE	 5.0

#define MID_A_FREQUENCY		440.0
#define MID_C_FREQUENCY		261.625565

/* object forward declaration */
typedef struct _LingotMainFrame LingotMainFrame;

// optionally we can use the following libraries
//#define LIB_FFTW
//#define LIBSNDOBJ

// simple try-catch simulation, do not use throw inside loops nor nest try-catch
// blocks
#define try exception = 0; do
#define throw(a) {exception = a;break;}
#define catch while (0); if (exception != 0)

// this option allows us to throw exception from loops, it contains a goto
// statement, but totally controlled. It fails when trying to indent code.
//#define try exception = 0;do
//#define throw(a) {exception = a;goto catch_label;}
//#define catch while (0);catch_label: if (exception != 0)

#endif
