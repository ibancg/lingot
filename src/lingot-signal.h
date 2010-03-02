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

#ifndef __LINGOT_SIGNAL_H__
#define __LINGOT_SIGNAL_H__

/*
 peak identification functions.
 */

#include "lingot-defs.h"
#include "lingot-config.h"

// returns noise threshold at a given frequency w.
FLT lingot_signal_get_noise_threshold(LingotConfig*, FLT w);

// returns if buffer has a peak at given index
int lingot_signal_is_peak(LingotConfig*, FLT* buffer, int index);

// returns the maximum index.
void lingot_signal_get_max(FLT *buffer, int N, int* Mi);

// returns the index of the peak that carries the fundamental freq.
int lingot_signal_get_fundamental_peak(LingotConfig*, FLT *x, FLT* y, int N);

// generates a Hamming window of N samples
void lingot_signal_hamming_window(int N, FLT* out);

#endif /*__LINGOT_SIGNAL_H__*/
