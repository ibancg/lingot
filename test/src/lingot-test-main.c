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

void lingot_config_test();
void lingot_config_scale_test();
void lingot_signal_test();
void lingot_core_test();

// TODO: lib?
#include "lingot-complex.c"
#include "lingot-msg.c"
#include "lingot-config-scale.c"
#include "lingot-config.c"
#include "lingot-audio.c"
#include "lingot-audio-alsa.c"
#include "lingot-audio-oss.c"
#include "lingot-audio-jack.c"
#include "lingot-audio-pulseaudio.c"
#include "lingot-fft.c"
#include "lingot-core.c"
#include "lingot-signal.c"
#include "lingot-filter.c"


int main(void) {

	lingot_config_test();
	lingot_config_scale_test();
	lingot_signal_test();
	lingot_core_test();

	return 0;
}
