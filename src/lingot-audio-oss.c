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

#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>

#include "lingot-error.h"
#include "lingot-defs.h"
#include "lingot-audio-oss.h"

const char* exception;

LingotAudioHandler* lingot_audio_oss_new(LingotCore* core) {

	int channels = 1;
	int rate = core->conf->sample_rate;
	int format = SAMPLE_FORMAT;
	char *fdsp = core->conf->audio_dev[AUDIO_SYSTEM_OSS];
	char error_message[100];

	LingotAudioHandler* audio = malloc(sizeof(LingotAudioHandler));
	sprintf(audio->device, "%s", "");

	audio->audio_system = AUDIO_SYSTEM_OSS;
	audio->dsp = open(fdsp, O_RDONLY);
	sprintf(audio->device, "%s", core->conf->audio_dev[AUDIO_SYSTEM_OSS]);

	try {

		if (audio->dsp < 0) {
			sprintf(error_message, "Unable to open audio device %s (%s)", fdsp,
					strerror(errno));
			throw(error_message);
		}

		//if (ioctl(audio->dsp, SOUND_PCM_READ_CHANNELS, &channels) < 0)
		if (ioctl(audio->dsp, SNDCTL_DSP_CHANNELS, &channels) < 0) {
			sprintf(error_message, "Error setting number of channels (%s)",
					strerror(errno));
			throw(error_message);
		}

		// sample size
		//if (ioctl(audio->dsp, SOUND_PCM_SETFMT, &format) < 0)
		if (ioctl(audio->dsp, SNDCTL_DSP_SETFMT, &format) < 0) {
			sprintf(error_message, "Error setting bits per sample (%s)",
					strerror(errno));
			throw(error_message);
		}

		int fragment_size = 1;
		int DMA_buffer_size = 512;
		int param = 0;

		for (param = 0; fragment_size < DMA_buffer_size; param++)
			fragment_size <<= 1;

		param |= 0x00ff0000;

		if (ioctl(audio->dsp, SNDCTL_DSP_SETFRAGMENT, &param) < 0) {
			sprintf(error_message, "Error setting DMA buffer size (%s)",
					strerror(errno));
			throw(error_message);
		}

		if (ioctl(audio->dsp, SNDCTL_DSP_SPEED, &rate) < 0) {
			sprintf(error_message, "Error setting sample rate (%s)", strerror(
					errno));
			throw(error_message);
		}

		audio->real_sample_rate = rate;
		audio->read_buffer = malloc(core->conf->read_buffer_size
				* sizeof(SAMPLE_TYPE));
		memset(audio->read_buffer, 0, core->conf->read_buffer_size
				* sizeof(SAMPLE_TYPE));

	} catch {
		close(audio->dsp);
		free(audio);
		audio = NULL;
		lingot_error_queue_push(exception);
	}

	return audio;
}

void lingot_audio_oss_destroy(LingotAudioHandler* audio) {
	if (audio != NULL) {
		close(audio->dsp);
		free(audio->read_buffer);
	}
}

int lingot_audio_oss_read(LingotAudioHandler* audio, LingotCore* core) {
	int i;

	read(audio->dsp, audio->read_buffer, core->conf->read_buffer_size
			* sizeof(SAMPLE_TYPE));

	// float point conversion
	for (i = 0; i < core->conf->read_buffer_size; i++)
		core->flt_read_buffer[i] = audio->read_buffer[i];

	return 0;
}

LingotAudioSystemProperties* lingot_audio_oss_get_audio_system_properties(
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

