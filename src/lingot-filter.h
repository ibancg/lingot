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

#ifndef LINGOT_FILTER_H
#define LINGOT_FILTER_H

#include <fcntl.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "lingot-defs.h"

/*
 digital filtering implementation.
 */

typedef struct {

    LINGOT_FLT* a;
    LINGOT_FLT* b; // coefs
    LINGOT_FLT* s; // status

    unsigned int N;

} lingot_filter_t;

void lingot_filter_new(lingot_filter_t*,
                       unsigned int Na, unsigned int Nb,
                       const LINGOT_FLT* a, const LINGOT_FLT* b);

void lingot_filter_reset(lingot_filter_t* filter);

/**
 * Design a Chebyshev Type I low pass filter with Rp dB of pass band ripple
 * with cutoff pi*wc radians.
 */
void lingot_filter_cheby_design(lingot_filter_t*, unsigned int order, LINGOT_FLT Rp, LINGOT_FLT wc);

void lingot_filter_destroy(lingot_filter_t*);

// Digital Filter Implementation II, in & out can overlap. Vector filtering
void lingot_filter_filter(lingot_filter_t*, unsigned int n, const LINGOT_FLT* in,
                          LINGOT_FLT* out);

// sample filtering
LINGOT_FLT lingot_filter_filter_sample(lingot_filter_t*, LINGOT_FLT in);

#ifdef __cplusplus
}
#endif

#endif
