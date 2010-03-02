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

#include "lingot-defs.h"
#include "lingot-audio.h"

#include "lingot-core.h"
#include "lingot-audio-oss.h"
#include "lingot-audio-alsa.h"
#include "lingot-audio-jack.h"

LingotAudio* lingot_audio_new(void* p) {

	LingotCore* core = (LingotCore*) p;
	switch (core->conf->audio_system) {
	case AUDIO_SYSTEM_OSS:
		return lingot_audio_oss_new(core);
	case AUDIO_SYSTEM_ALSA:
		return lingot_audio_alsa_new(core);
	case AUDIO_SYSTEM_JACK:
		return lingot_audio_jack_new(core);
	default:
		return NULL;
	}
}

void lingot_audio_destroy(LingotAudio* audio, void* p) {
	if (audio != NULL) {
		switch (audio->audio_system) {
		case AUDIO_SYSTEM_OSS:
			lingot_audio_oss_destroy(audio);
			break;
		case AUDIO_SYSTEM_ALSA:
			lingot_audio_alsa_destroy(audio);
			break;
		case AUDIO_SYSTEM_JACK:
			lingot_audio_jack_destroy(audio);
			break;
		default:
			perror("unknown audio system\n");
		}

		free(audio);
	}
}

int lingot_audio_read(LingotAudio* audio, void* p) {
	LingotCore* core = (LingotCore*) p;
	if (audio != NULL)
		switch (audio->audio_system) {
		case AUDIO_SYSTEM_OSS:
			return lingot_audio_oss_read(audio, core);
		case AUDIO_SYSTEM_ALSA:
			return lingot_audio_alsa_read(audio, core);
		case AUDIO_SYSTEM_JACK:
			return lingot_audio_jack_read(audio, core);
		default:
			perror("unknown audio system\n");
		}

	return -1;
}

