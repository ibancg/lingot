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

#ifndef __LINGOT_AUDIO_H__
#define __LINGOT_AUDIO_H__

#ifdef ALSA
#include <alsa/asoundlib.h>
#endif

#ifdef JACK
#include <jack/jack.h>
#endif

#include "lingot-config.h"

typedef struct _LingotAudioHandler LingotAudioHandler;

struct _LingotAudioHandler {

	int audio_system;
	char device[100];

#ifdef ALSA
	snd_pcm_t *capture_handle;
#endif

	int dsp; // file handler.
	SAMPLE_TYPE* read_buffer;

#	ifdef JACK
	jack_port_t *jack_input_port;
	jack_client_t *jack_client;
	int nframes;
#	endif

	char error_message[100];
	unsigned int real_sample_rate;
};

typedef struct _LingotAudioSystemProperties LingotAudioSystemProperties;

struct _LingotAudioSystemProperties {

	int forced_sample_rate; // tells whether the sample rate can be changed

	int n_sample_rates; // number of available sample rates
	int* sample_rates; // sample rates

	int n_devices; // number of available devices
	char** devices; // devices
};

LingotAudioSystemProperties* lingot_audio_get_audio_system_properties(
		audio_system_t audio_system);
void lingot_audio_audio_system_properties_destroy(LingotAudioSystemProperties*);

// creates an audio handler
LingotAudioHandler* lingot_audio_new(void*);

// destroys an audio handler
void lingot_audio_destroy(LingotAudioHandler*, void*);

// reads a new piece of signal
int lingot_audio_read(LingotAudioHandler*, void*);

#endif
