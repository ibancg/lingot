/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2009  Ibán Cereijo Graña, Jairo Chapela Martínez.
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

LingotAudio* lingot_audio_alsa_new(LingotCore* core) {
#	ifdef ALSA
	snd_pcm_hw_params_t *hw_params;
	int err;
	LingotAudio* audio = malloc(sizeof(LingotAudio));

	audio->audio_system = AUDIO_SYSTEM_ALSA;

	if ((err = snd_pcm_open(&audio->capture_handle, "default",
							SND_PCM_STREAM_CAPTURE, 0)) < 0) {
		fprintf(stderr, "cannot open audio device %s (%s)\n", fdsp,
				snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		fprintf(stderr, "cannot allocate hardware parameter structure (%s)\n",
				snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params_any(audio->capture_handle, hw_params)) < 0) {
		fprintf(stderr,
				"cannot initialize hardware parameter structure (%s)\n", snd_strerror(
						err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params_set_access(audio->capture_handle, hw_params,
							SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		fprintf(stderr, "cannot set access type (%s)\n", snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params_set_format(audio->capture_handle, hw_params,
							SND_PCM_FORMAT_S16_LE)) < 0) {
		fprintf(stderr, "cannot set sample format (%s)\n", snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params_set_rate_near(audio->capture_handle,
							hw_params, &rate, 0)) < 0) {
		fprintf(stderr, "cannot set sample rate (%s)\n", snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params_set_channels(audio->capture_handle, hw_params,
							channels)) < 0) {
		fprintf(stderr, "cannot set channel count (%s)\n", snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params(audio->capture_handle, hw_params)) < 0) {
		fprintf(stderr, "cannot set parameters (%s)\n", snd_strerror(err));
		exit(1);
	}

	snd_pcm_hw_params_free(hw_params);

	if ((err = snd_pcm_prepare(audio->capture_handle)) < 0) {
		fprintf(stderr, "cannot prepare audio interface for use (%s)\n",
				snd_strerror(err));
		exit(1);
	}

	return audio;
#	else
	return NULL;
#	endif
}

void lingot_audio_alsa_destroy(LingotAudio* audio) {
#	ifdef ALSA
	snd_pcm_close(audio->capture_handle);
#	endif
}

int lingot_audio_alsa_read(LingotAudio* audio, LingotCore* core) {
#	ifdef ALSA
	snd_pcm_readi(audio->capture_handle, core->read_buffer, core->conf->read_buffer_size
			* sizeof(SAMPLE_TYPE));

	// float point conversion
	for (i = 0; i < core->conf->read_buffer_size; i++)
	core->flt_read_buffer[i] = core->read_buffer[i];
#	endif

	return 0;
}
