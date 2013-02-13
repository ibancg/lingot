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

#include "lingot-test.h"

#include "errno.h"
#include "lingot-audio.h"
#include "lingot-audio-alsa.h"
#include "lingot-audio-oss.h"
#include "lingot-audio-jack.h"
#include "lingot-audio-pulseaudio.h"
#include "lingot-fft.h"

#include "lingot-core.h"

void lingot_core_test() {

	FLT multiplier1 = 0.0;
	FLT multiplier2 = 0.0;

	int rel = lingot_core_frequencies_related(100.0, 150.001, 20.0, &multiplier1,
			&multiplier2);
	CU_ASSERT_EQUAL(rel, 1);
	CU_ASSERT_EQUAL(multiplier1, 0.5);

	rel = lingot_core_frequencies_related(100.0, 200.01, 20.0, &multiplier1,
			&multiplier2);
	CU_ASSERT_EQUAL(rel, 1);
	CU_ASSERT_EQUAL(multiplier1, 1.0);

	rel = lingot_core_frequencies_related(200.0, 100.01, 20.0, &multiplier1,
			&multiplier2);
	CU_ASSERT_EQUAL(rel, 1);
	CU_ASSERT_EQUAL(multiplier1, 0.5);

	rel = lingot_core_frequencies_related(100.0, 150.001, 70.0, &multiplier1,
			&multiplier2);
	CU_ASSERT_EQUAL(rel, 0);

	rel = lingot_core_frequencies_related(22.788177, 114.008917, 15.0,
			&multiplier1, &multiplier2);
	CU_ASSERT_EQUAL(rel, 1);
	CU_ASSERT_EQUAL(multiplier1, 1.0);
	CU_ASSERT_EQUAL(multiplier2, 0.2);


	rel = lingot_core_frequencies_related(97.959328, 48.977020, 15.0,
			&multiplier1, &multiplier2);
	CU_ASSERT_EQUAL(rel, 1);
	CU_ASSERT_EQUAL(multiplier1, 0.5);
	CU_ASSERT_EQUAL(multiplier2, 1.0);
}
