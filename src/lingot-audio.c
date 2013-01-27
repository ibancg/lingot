/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2013  Ibán Cereijo Graña, Jairo Chapela Martínez.
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
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include "lingot-defs.h"
#include "lingot-audio.h"

#include "lingot-core.h"
#include "lingot-audio-oss.h"
#include "lingot-audio-alsa.h"
#include "lingot-audio-jack.h"
#include "lingot-audio-pulseaudio.h"

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
	case AUDIO_SYSTEM_PULSEAUDIO:
		result = lingot_audio_pulseaudio_new(device, sample_rate);
		break;
	}

	if (result != NULL) {
		// audio source read in floating point format.
		result->flt_read_buffer = malloc(
				result->read_buffer_size * sizeof(FLT));
		memset(result->flt_read_buffer, 0,
				result->read_buffer_size * sizeof(FLT));
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
		case AUDIO_SYSTEM_PULSEAUDIO:
			lingot_audio_pulseaudio_destroy(audio);
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

	if (audio != NULL
	)
		switch (audio->audio_system) {
		case AUDIO_SYSTEM_OSS:
			result = lingot_audio_oss_read(audio);
			break;
		case AUDIO_SYSTEM_ALSA:
			result = lingot_audio_alsa_read(audio);
			break;
		case AUDIO_SYSTEM_PULSEAUDIO:
			result = lingot_audio_pulseaudio_read(audio);
			break;
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
	case AUDIO_SYSTEM_PULSEAUDIO:
		result = lingot_audio_pulseaudio_get_audio_system_properties(
				audio_system);
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
	if (properties->devices != NULL) {
		for (i = 0; i < properties->n_devices; i++) {
			if (properties->devices[i] != NULL) {
				free(properties->devices[i]);
			}
		}
	}

	if (properties->sample_rates != NULL
	)
		free(properties->sample_rates);
	if (properties->devices != NULL
	)
		free(properties->devices);
}

void lingot_audio_run_reading_thread(LingotAudioHandler* audio) {

	int samples_read = 0;

	while (audio->running) {
		// process new data block.
		samples_read = lingot_audio_read(audio);

		if (samples_read < 0) {
			audio->running = 0;
			audio->interrupted = 1;
		} else {
			audio->process_callback(audio->flt_read_buffer, samples_read,
					audio->process_callback_arg);
		}
	}

	pthread_mutex_lock(&audio->thread_input_read_mutex);
	pthread_cond_broadcast(&audio->thread_input_read_cond);
	pthread_mutex_unlock(&audio->thread_input_read_mutex);

}

int lingot_audio_start(LingotAudioHandler* audio) {

	int result = 0;

	switch (audio->audio_system) {
	case AUDIO_SYSTEM_JACK:
		result = lingot_audio_jack_start(audio);
		break;
	default:
		pthread_attr_init(&audio->thread_input_read_attr);

		// detached thread.
		//  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_mutex_init(&audio->thread_input_read_mutex, NULL);
		pthread_cond_init(&audio->thread_input_read_cond, NULL);
		pthread_attr_init(&audio->thread_input_read_attr);
		pthread_create(&audio->thread_input_read,
				&audio->thread_input_read_attr,
				(void* (*)(void*)) lingot_audio_run_reading_thread, audio);
		break;
	}

	if (result == 0) {
		audio->running = 1;
	}

	return result;
}

// function invoked when the audio thread must be cancelled
void lingot_audio_cancel(LingotAudioHandler* audio) {
	fprintf(stderr, "warning: cancelling audio thread\n");
	switch (audio->audio_system) {
	case AUDIO_SYSTEM_PULSEAUDIO:
		lingot_audio_pulseaudio_cancel(audio);
		break;
	default:
		break;
	}
}

void lingot_audio_stop(LingotAudioHandler* audio) {
	void* thread_result;

	int result;
	struct timeval tout, tout_abs;
	struct timespec tout_tspec;

	gettimeofday(&tout_abs, NULL);
	tout.tv_sec = 0;
	tout.tv_usec = 100000;

	if (audio->running == 1) {
		audio->running = 0;
		switch (audio->audio_system) {
		case AUDIO_SYSTEM_JACK:
			lingot_audio_jack_stop(audio);
			break;
//		case AUDIO_SYSTEM_PULSEAUDIO:
//			pthread_join(audio->thread_input_read, &thread_result);
//			pthread_attr_destroy(&audio->thread_input_read_attr);
//			pthread_mutex_destroy(&audio->thread_input_read_mutex);
//			pthread_cond_destroy(&audio->thread_input_read_cond);
//			break;
		default:
			timeradd(&tout, &tout_abs, &tout_abs);
			tout_tspec.tv_sec = tout_abs.tv_sec;
			tout_tspec.tv_nsec = 1000 * tout_abs.tv_usec;

			// watchdog timer
			pthread_mutex_lock(&audio->thread_input_read_mutex);
			result = pthread_cond_timedwait(&audio->thread_input_read_cond,
					&audio->thread_input_read_mutex, &tout_tspec);
			pthread_mutex_unlock(&audio->thread_input_read_mutex);

			if (result == ETIMEDOUT) {
				pthread_cancel(audio->thread_input_read);
				lingot_audio_cancel(audio);
			} else {
				pthread_join(audio->thread_input_read, &thread_result);
			}
			pthread_attr_destroy(&audio->thread_input_read_attr);
			pthread_mutex_destroy(&audio->thread_input_read_mutex);
			pthread_cond_destroy(&audio->thread_input_read_cond);
			break;
		}
	}
}

