/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2013  Ibán Cereijo Graña
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

#include <assert.h>

#include "errno.h"
#include "lingot-audio.h"
#include "lingot-audio.c"
#include "lingot-audio-alsa.h"
#include "lingot-audio-alsa.c"
#include "lingot-audio-oss.h"
#include "lingot-audio-oss.c"
#include "lingot-audio-jack.h"
#include "lingot-audio-jack.c"
#include "lingot-audio-pulseaudio.h"
#include "lingot-audio-pulseaudio.c"
#include "lingot-fft.h"
#include "lingot-fft.c"

#include "lingot-core.h"
#include "lingot-core.c"

void lingot_core_test() {

	FLT multiplier = 0.0;
	FLT multiplier2 = 0.0;

	int rel = lingot_core_frequencies_related(100.0, 150.1, 20.0, &multiplier,
			&multiplier2);
	assert(rel == 1);
	assert(multiplier == 0.5);

	rel = lingot_core_frequencies_related(100.0, 200.1, 20.0, &multiplier,
			&multiplier2);
	assert(rel == 1);
	assert(multiplier == 1.0);

	rel = lingot_core_frequencies_related(200.0, 100.1, 20.0, &multiplier,
			&multiplier2);
	assert(rel == 1);
	assert(multiplier == 0.5);

	rel = lingot_core_frequencies_related(100.0, 150.1, 70.0, &multiplier,
			&multiplier2);
	assert(rel == 1);

	rel = lingot_core_frequencies_related(22.788177, 114.008917, 15.0,
			&multiplier, &multiplier2);
	assert(rel == 1);
	assert(multiplier == 1.0);
	assert(multiplier2 == 0.2);


	rel = lingot_core_frequencies_related(97.959328, 48.977020, 15.0,
			&multiplier, &multiplier2);
	assert(rel == 1);
	assert(multiplier == 1.0);
	assert(multiplier2 == 2.0);

}
