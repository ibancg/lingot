/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2011  Ibán Cereijo Graña, Jairo Chapela Martínez.
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

#ifndef _LINGOT_COMPLEX_H_
#define _LINGOT_COMPLEX_H_

#include <math.h>
#include "lingot-defs.h"

// single complex arithmetic  :)

typedef struct _LingotComplex LingotComplex;

struct _LingotComplex
  {
    FLT r;
    FLT i;
  };

void lingot_complex_add(LingotComplex* a, LingotComplex* b, LingotComplex* c);
void lingot_complex_sub(LingotComplex* a, LingotComplex* b, LingotComplex* c);
void lingot_complex_mul(LingotComplex* a, LingotComplex* b, LingotComplex* c);
void lingot_complex_div(LingotComplex* a, LingotComplex* b, LingotComplex* c);

#endif
