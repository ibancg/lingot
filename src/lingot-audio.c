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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lingot-defs.h"
#include "lingot-audio.h"

#include "lingot-core.h"
#include "lingot-audio-oss.h"
#include "lingot-audio-alsa.h"
#include "lingot-audio-jack.h"

LingotAudioHandler* lingot_audio_new(audio_system_t audio_system, char* device,
		int sample_rate, LingotAudioProcessCallback process_callback,
		void *process_callback_arg) {

	LingotAudioHandler* result = NULL;

	switch (audio_system) {
	case AUDIO_SYSTEM_OSS:
		result = lingot_audio_oss_new(device, sample_rate);
		break;
	case AUDIO_SYSTEM_ALSA:
		result = lingot_audio_alsa_new(device, sample_rate);
		break;
	case AUDIO_SYSTEM_JACK:
		result = lingot_audio_jack_new(device, sample_rate);
		break;
	}

	if (result != NULL) {
		// audio source read in floating point format.
		result->flt_read_buffer
				= malloc(result->read_buffer_size * sizeof(FLT));
		memset(result->flt_read_buffer, 0, result->read_buffer_size
				* sizeof(FLT));
		result->process_callback = process_callback;
		result->process_callback_arg = process_callback_arg;
		result->interrupted = 0;
		result->running = 0;
	}

	return result;
}

void lingot_audio_destroy(LingotAudioHandler* audio) {
	if (audio != NULL) {

		free(audio->flt_read_buffer);

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
			break;
		}

		free(audio);
	}
}

int lingot_audio_read(LingotAudioHandler* audio) {
	int result = -1;

	if (audio != NULL)
		switch (audio->audio_system) {
		case AUDIO_SYSTEM_OSS:
			result = lingot_audio_oss_read(audio);
			break;
		case AUDIO_SYSTEM_ALSA:
			result = lingot_audio_alsa_read(audio);
			break;
			//		case AUDIO_SYSTEM_JACK:
			//			result = lingot_audio_jack_read(audio);
			//			break;
		default:
			perror("unknown audio system\n");
			result = -1;
			break;
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
		break;
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

void lingot_audio_run_reading_thread(LingotAudioHandler* audio) {

	int read_status = 0;

	while (audio->running) {
		// process new data block.
		read_status = lingot_audio_read(audio);

		if (read_status == 0) {
			audio->process_callback(audio->flt_read_buffer,
					audio->read_buffer_size, audio->process_callback_arg);
		} else {
			audio->running = 0;
			audio->interrupted = 1;
		}
	}
}

int lingot_audio_start(LingotAudioHandler* audio) {

	int result = 0;

	if (audio->audio_system != AUDIO_SYSTEM_JACK) {
		pthread_attr_init(&audio->thread_input_read_attr);

		// detached thread.
		//  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&audio->thread_input_read,
				&audio->thread_input_read_attr,
				(void* (*)(void*)) lingot_audio_run_reading_thread, audio);
	} else {
		result = lingot_audio_jack_start(audio);
	}

	if (result == 0) {
		audio->running = 1;
	}

	return result;
}

void lingot_audio_stop(LingotAudioHandler* audio) {
	void* thread_result;

	if (audio->running == 1) {
		audio->running = 0;
		// thread cancellation
		if (audio->audio_system != AUDIO_SYSTEM_JACK) {
			pthread_cancel(audio->thread_input_read);
			pthread_join(audio->thread_input_read, &thread_result);
			pthread_attr_destroy(&audio->thread_input_read_attr);
		} else {
			lingot_audio_jack_stop(audio);
		}
	}
}

