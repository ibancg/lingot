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
#include "lingot-audio-alsa.h"
#include "lingot-i18n.h"
#include "lingot-error.h"

LingotAudio* lingot_audio_alsa_new(LingotCore* core) {

	LingotAudio* audio = NULL;

#	ifdef ALSA
	snd_pcm_hw_params_t *hw_params;
	int err;
	char error_message[100];
	unsigned int channels = 1;

	audio = malloc(sizeof(LingotAudio));

	// ALSA allocates some mem to load its config file when we call
	// snd_card_next. Now that we're done getting the info, let's tell ALSA
	// to unload the info and free up that mem
	snd_config_update_free_global();

	audio->audio_system = AUDIO_SYSTEM_ALSA;

	if ((err = snd_pcm_open(&audio->capture_handle, core->conf->audio_dev_alsa,
			SND_PCM_STREAM_CAPTURE, 0)) < 0) {
		sprintf(error_message, "cannot open audio device %s (%s)\n",
				core->conf->audio_dev_alsa, snd_strerror(err));
		lingot_error_queue_push(error_message);
		free(audio);
		return NULL;
	}

	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		sprintf(error_message,
				"cannot allocate hardware parameter structure (%s)\n",
				snd_strerror(err));
		lingot_error_queue_push(error_message);
		free(audio);
		return NULL;
	}

	if ((err = snd_pcm_hw_params_any(audio->capture_handle, hw_params)) < 0) {
		sprintf(error_message,
				"cannot initialize hardware parameter structure (%s)\n",
				snd_strerror(err));
		lingot_error_queue_push(error_message);
		free(audio);
		snd_pcm_hw_params_free(hw_params);
		return NULL;
	}

	/* set hardware resampling */
	//	err = snd_pcm_hw_params_set_rate_resample(audio->capture_handle, hw_params,
	//			resample);
	//	if (err < 0) {
	//		printf("Resampling setup failed for playback: %s\n", snd_strerror(err));
	//		exit(1);
	//	}

	if ((err = snd_pcm_hw_params_set_access(audio->capture_handle, hw_params,
			SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		sprintf(error_message, "cannot set access type (%s)\n", snd_strerror(
				err));
		lingot_error_queue_push(error_message);
		free(audio);
		snd_pcm_hw_params_free(hw_params);
		return NULL;
	}

	if ((err = snd_pcm_hw_params_set_format(audio->capture_handle, hw_params,
			SND_PCM_FORMAT_S16_LE)) < 0) {
		sprintf(error_message, "cannot set sample format (%s)\n", snd_strerror(
				err));
		lingot_error_queue_push(error_message);
		free(audio);
		snd_pcm_hw_params_free(hw_params);
		return NULL;
	}

	if ((err = snd_pcm_hw_params_set_rate_near(audio->capture_handle,
			hw_params, &core->conf->sample_rate, 0)) < 0) {
		sprintf(error_message, "cannot set sample rate (%s)\n", snd_strerror(
				err));
		lingot_error_queue_push(error_message);
		free(audio);
		snd_pcm_hw_params_free(hw_params);
		return NULL;
	}

	if ((err = snd_pcm_hw_params_set_channels(audio->capture_handle, hw_params,
			channels)) < 0) {
		sprintf(error_message, "cannot set channel count (%s)\n", snd_strerror(
				err));
		lingot_error_queue_push(error_message);
		free(audio);
		snd_pcm_hw_params_free(hw_params);
		return NULL;
	}

	if ((err = snd_pcm_hw_params(audio->capture_handle, hw_params)) < 0) {
		sprintf(error_message, "cannot set parameters (%s)\n",
				snd_strerror(err));
		lingot_error_queue_push(error_message);
		free(audio);
		snd_pcm_hw_params_free(hw_params);
		return NULL;
	}

	snd_pcm_hw_params_free(hw_params);

	if ((err = snd_pcm_prepare(audio->capture_handle)) < 0) {
		sprintf(error_message, "cannot prepare audio interface for use (%s)\n",
				snd_strerror(err));
		lingot_error_queue_push(error_message);
		free(audio);
		return NULL;
	}

	audio->read_buffer = malloc(core->conf->read_buffer_size
			* sizeof(SAMPLE_TYPE));
	memset(audio->read_buffer, 0, core->conf->read_buffer_size
			* sizeof(SAMPLE_TYPE));

#	else
	lingot_error_queue_push(
			_("The application has not been built with ALSA support"));
#	endif

	return audio;
}

void lingot_audio_alsa_destroy(LingotAudio* audio) {
#	ifdef ALSA
	snd_pcm_close(audio->capture_handle);
#	endif
	if (audio != NULL) {
		free(audio->read_buffer);
	}
}

int lingot_audio_alsa_read(LingotAudio* audio, LingotCore* core) {
	int i;
	int temp_sret;

#	ifdef ALSA
	temp_sret = snd_pcm_readi(audio->capture_handle, audio->read_buffer,
			core->conf->read_buffer_size);

	// float point conversion
	for (i = 0; i < core->conf->read_buffer_size; i++) {
		core->flt_read_buffer[i] = audio->read_buffer[i];
	}
#	endif

	return 0;
}
