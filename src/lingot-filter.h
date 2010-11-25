//-*- C++ -*-
/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2010  Ibán Cereijo Graña, Jairo Chapela Martínez.
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

#ifndef __LINGOT_FILTER_H__
#define __LINGOT_FILTER_H__

#include <fcntl.h>
#include <stdlib.h>

#include "lingot-defs.h"

/*
 digital filtering implementation.
 */

typedef struct _LingotFilter LingotFilter;

struct _LingotFilter
  {

    FLT* a;
    FLT* b; // coefs
    FLT* s; // status

    unsigned int N;

  };

LingotFilter
    * lingot_filter_new(unsigned int Na, unsigned int Nb, FLT* a, FLT* b);

/**
 * Design a Chebyshev type I low pass filter with Rp dB of pass band ripple
 * with cutoff pi*wc radians.
 */
LingotFilter* lingot_filter_cheby_design(unsigned int order, FLT Rp, FLT wc);

void lingot_filter_destroy(LingotFilter*);

// Digital Filter Implementation II, in & out overlapables. Vector filtering
void lingot_filter_filter(LingotFilter*, unsigned int n, FLT* in, FLT* out);

// sample filtering
FLT lingot_filter_filter_sample(LingotFilter*, FLT in);

#endif
