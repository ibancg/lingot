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
 * along with lingot; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef LINGOT_GAUGE_H
#define LINGOT_GAUGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lingot-defs.h"
#include "lingot-filter.h"

/*
 * Implements the dynamic behaviour of the gauge with a digital filter.
 */

typedef struct {
    lingot_filter_t filter;
    FLT position;
} lingot_gauge_t;

void lingot_gauge_new(lingot_gauge_t*, FLT gauge_rate, FLT initial_position);
void lingot_gauge_destroy(lingot_gauge_t*);
void lingot_gauge_compute(lingot_gauge_t*, FLT position);

#ifdef __cplusplus
}
#endif

#endif /*LINGOT_GAUGE_H*/
