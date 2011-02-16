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

#include "lingot-defs.h"
#include "lingot-audio-alsa.h"
#include "lingot-i18n.h"
#include "lingot-error.h"

LingotAudioHandler* lingot_audio_alsa_new(char* device, int sample_rate) {

	LingotAudioHandler* audio = NULL;
	const char* exception;

#	ifdef ALSA
	snd_pcm_hw_params_t* hw_params = NULL;
	int err;
	char error_message[1000];
	unsigned int channels = 1;

	audio = malloc(sizeof(LingotAudioHandler));
	audio->read_buffer = NULL;
	audio->audio_system = AUDIO_SYSTEM_ALSA;
	audio->read_buffer_size = 128; // TODO: ?
	audio->running = 0;

	// ALSA allocates some mem to load its config file when we call
	// snd_card_next. Now that we're done getting the info, let's tell ALSA
	// to unload the info and free up that mem
	snd_config_update_free_global();

	audio->capture_handle = NULL;

	try {
		if ((err = snd_pcm_open(&audio->capture_handle, device,
				SND_PCM_STREAM_CAPTURE, 0)) < 0) {
			sprintf(error_message, "cannot open audio device %s (%s)\n",
					device, snd_strerror(err));
			throw(error_message);
		}

		strcpy(audio->device, device);

		if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
			sprintf(error_message,
					"cannot allocate hardware parameter structure (%s)\n",
					snd_strerror(err));
			throw(error_message);
		}

		if ((err = snd_pcm_hw_params_any(audio->capture_handle, hw_params)) < 0) {
			sprintf(error_message,
					"cannot initialize hardware parameter structure (%s)\n",
					snd_strerror(err));
			throw(error_message);
		}

		if ((err = snd_pcm_hw_params_set_access(audio->capture_handle,
				hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
			sprintf(error_message, "cannot set access type (%s)\n",
					snd_strerror(err));
			throw(error_message);
		}

		if ((err = snd_pcm_hw_params_set_format(audio->capture_handle,
				hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
			sprintf(error_message, "cannot set sample format (%s)\n",
					snd_strerror(err));
			throw(error_message);
		}

		unsigned int rate = sample_rate;

		if ((err = snd_pcm_hw_params_set_rate_near(audio->capture_handle,
				hw_params, &rate, 0)) < 0) {
			sprintf(error_message, "cannot set sample rate (%s)\n",
					snd_strerror(err));
			throw(error_message);
		}

		audio->real_sample_rate = rate;

		if ((err = snd_pcm_hw_params_set_channels(audio->capture_handle,
				hw_params, channels)) < 0) {
			sprintf(error_message, "cannot set channel count (%s)\n",
					snd_strerror(err));
			throw(error_message);
		}

		if ((err = snd_pcm_hw_params(audio->capture_handle, hw_params)) < 0) {
			sprintf(error_message, "cannot set parameters (%s)\n",
					snd_strerror(err));
			throw(error_message);
		}

		if ((err = snd_pcm_prepare(audio->capture_handle)) < 0) {
			sprintf(error_message,
					"cannot prepare audio interface for use (%s)\n",
					snd_strerror(err));
			throw(error_message);
		}

		audio->read_buffer = malloc(channels * audio->read_buffer_size
				* sizeof(SAMPLE_TYPE));
		memset(audio->read_buffer, 0, audio->read_buffer_size
				* sizeof(SAMPLE_TYPE));
	} catch {
		if (audio->capture_handle != NULL)
			snd_pcm_close(audio->capture_handle);
		free(audio);
		audio = NULL;
		lingot_error_queue_push(exception);
	}

	if (hw_params != NULL)
		snd_pcm_hw_params_free(hw_params);

#	else
	lingot_error_queue_push(
			_("The application has not been built with ALSA support"));
#	endif

	return audio;
}

void lingot_audio_alsa_destroy(LingotAudioHandler* audio) {
#	ifdef ALSA
	if (audio != NULL) {
		snd_pcm_close(audio->capture_handle);
		free(audio->read_buffer);
	}
#	endif
}

int lingot_audio_alsa_read(LingotAudioHandler* audio) {
	int temp_sret;
	int i;

#	ifdef ALSA
	temp_sret = snd_pcm_readi(audio->capture_handle, audio->read_buffer,
			audio->read_buffer_size);

	//	if (rand() < 0.001*RAND_MAX)
	//		temp_sret = 0;

	if (temp_sret != audio->read_buffer_size) {
		char buff[100];
		sprintf(buff, "read from audio interface failed (%s)", snd_strerror(
				temp_sret));
		printf("%s", buff);
		lingot_error_queue_push(buff);

		return -1;
	}

	// float point conversion
	for (i = 0; i < audio->read_buffer_size; i++) {
		audio->flt_read_buffer[i] = audio->read_buffer[i];
	}

#	endif

	return 0;
}

LingotAudioSystemProperties* lingot_audio_alsa_get_audio_system_properties(
		audio_system_t audio_system) {

	LingotAudioSystemProperties* result =
			(LingotAudioSystemProperties*) malloc(1
					* sizeof(LingotAudioSystemProperties));

	// TODO
	result->forced_sample_rate = 0;
	result->n_devices = 0;
	result->devices = NULL;

	result->n_sample_rates = 4;
	result->sample_rates = malloc(result->n_sample_rates * sizeof(int));
	result->sample_rates[0] = 8000;
	result->sample_rates[1] = 11025;
	result->sample_rates[2] = 22050;
	result->sample_rates[3] = 44100;

	return result;
}

