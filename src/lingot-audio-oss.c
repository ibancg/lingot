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

LingotAudio* lingot_audio_oss_new(LingotCore* core) {

	int channels = 1;
	int rate = core->conf->sample_rate;
	int format = SAMPLE_FORMAT;
	char *fdsp = core->conf->audio_dev;
	char error_message[100];

	LingotAudio* audio = malloc(sizeof(LingotAudio));
	audio->audio_system = AUDIO_SYSTEM_OSS;
	audio->dsp = open(fdsp, O_RDONLY);
	if (audio->dsp < 0) {
		sprintf(error_message, "Unable to open audio device %s", fdsp);
		lingot_error_queue_push(error_message);
		free(audio);
		return NULL;
	}

	//if (ioctl(audio->dsp, SOUND_PCM_READ_CHANNELS, &channels) < 0)
	if (ioctl(audio->dsp, SNDCTL_DSP_CHANNELS, &channels) < 0) {
		lingot_error_queue_push("Error setting number of channels");
		free(audio);
		return NULL;
	}

	// sample size
	//if (ioctl(audio->dsp, SOUND_PCM_SETFMT, &format) < 0)
	if (ioctl(audio->dsp, SNDCTL_DSP_SETFMT, &format) < 0) {
		lingot_error_queue_push("Error setting bits per sample");
		free(audio);
		return NULL;
	}

	int fragment_size = 1;
	int DMA_buffer_size = 512;
	int param = 0;

	for (param = 0; fragment_size < DMA_buffer_size; param++)
		fragment_size <<= 1;

	param |= 0x00ff0000;

	if (ioctl(audio->dsp, SNDCTL_DSP_SETFRAGMENT, &param) < 0) {
		lingot_error_queue_push("Error setting DMA buffer size");
		free(audio);
		return NULL;
	}

	if (ioctl(audio->dsp, SNDCTL_DSP_SPEED, &rate) < 0) {
		lingot_error_queue_push("Error setting sample rate");
		free(audio);
		return NULL;
	}

	audio->read_buffer = malloc(core->conf->read_buffer_size
			* sizeof(SAMPLE_TYPE));
	memset(audio->read_buffer, 0, core->conf->read_buffer_size
			* sizeof(SAMPLE_TYPE));

	return audio;
}

void lingot_audio_oss_destroy(LingotAudio* audio) {
	if (audio != NULL) {
		close(audio->dsp);
		free(audio->read_buffer);
	}
}

int lingot_audio_oss_read(LingotAudio* audio, LingotCore* core) {
	int i;

	read(audio->dsp, audio->read_buffer, core->conf->read_buffer_size
			* sizeof(SAMPLE_TYPE));

	// float point conversion
	for (i = 0; i < core->conf->read_buffer_size; i++)
		core->flt_read_buffer[i] = audio->read_buffer[i];

	return 0;
}
