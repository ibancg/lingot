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

LingotAudioHandler* lingot_audio_new(void* p) {

	LingotCore* core = (LingotCore*) p;
	LingotAudioHandler* result = NULL;

	switch (core->conf->audio_system) {
	case AUDIO_SYSTEM_OSS:
		result = lingot_audio_oss_new(core);
		break;
	case AUDIO_SYSTEM_ALSA:
		result = lingot_audio_alsa_new(core);
		break;
	case AUDIO_SYSTEM_JACK:
		result = lingot_audio_jack_new(core);
		break;
	}

	return result;
}

void lingot_audio_destroy(LingotAudioHandler* audio, void* p) {
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

int lingot_audio_read(LingotAudioHandler* audio, void* p) {
	LingotCore* core = (LingotCore*) p;
	int result;

	if (audio != NULL)
		switch (audio->audio_system) {
		case AUDIO_SYSTEM_OSS:
			result = lingot_audio_oss_read(audio, core);
			break;
		case AUDIO_SYSTEM_ALSA:
			result = lingot_audio_alsa_read(audio, core);
			break;
		case AUDIO_SYSTEM_JACK:
			result = lingot_audio_jack_read(audio, core);
			break;
		default:
			perror("unknown audio system\n");
			result = -1;
		}

	return result;
}

LingotAudioSystemProperties* lingot_audio_get_audio_system_properties(
		audio_system_t audio_system) {
	LingotAudioSystemProperties* result;

	switch (audio_system) {
	case AUDIO_SYSTEM_OSS:
		result = lingot_audio_oss_get_audio_system_properties(audio_system);
		break;
	case AUDIO_SYSTEM_ALSA:
		result = lingot_audio_alsa_get_audio_system_properties(audio_system);
		break;
	case AUDIO_SYSTEM_JACK:
		result = lingot_audio_jack_get_audio_system_properties(audio_system);
		break;
	default:
		perror("unknown audio system\n");
		result = NULL;
	}

	return result;
}

void lingot_audio_audio_system_properties_destroy(
		LingotAudioSystemProperties* properties) {

	int i;
	for (i = 0; i < properties->n_devices; i++) {
		free(properties->devices[i]);
	}

	if (properties->sample_rates != NULL)
		free(properties->sample_rates);
	if (properties->devices != NULL)
		free(properties->devices);
}
