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

#ifndef __LINGOT_AUDIO_H__
#define __LINGOT_AUDIO_H__

#ifdef ALSA
#include <alsa/asoundlib.h>
#endif

#ifdef JACK
#include <jack/jack.h>
#endif

#include "lingot-config.h"

typedef void (*LingotAudioProcessCallback)(FLT* read_buffer,
		int read_buffer_size, void *arg);

typedef void (*LingotAudioShutdownCallback)(void *arg);

typedef struct _LingotAudioHandler LingotAudioHandler;

struct _LingotAudioHandler {

	int audio_system;
	char device[100];

	LingotAudioProcessCallback process_callback;
	void* process_callback_arg;

	LingotAudioShutdownCallback shutdown_callback;
	void* shutdown_callback_arg;

#ifdef ALSA
	snd_pcm_t *capture_handle;
#endif

	int dsp; // file handler.
	int read_buffer_size;
	SAMPLE_TYPE* read_buffer;
	FLT* flt_read_buffer;

#	ifdef JACK
	jack_port_t *jack_input_port;
	jack_client_t *jack_client;
	int nframes;
#	endif

	//	char error_message[100];
	unsigned int real_sample_rate;

	// pthread-related  member variables
	pthread_t thread_input_read;
	pthread_attr_t thread_input_read_attr;

	int running;
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
LingotAudioHandler*
lingot_audio_new(audio_system_t audio_system, char* device, int sample_rate,
		LingotAudioProcessCallback process_callback,
		void *process_callback_arg,
		LingotAudioShutdownCallback shutdown_callback,
		void* shutdown_callback_arg);

// destroys an audio handler
void lingot_audio_destroy(LingotAudioHandler*);

// reads a new piece of signal
//int lingot_audio_read(LingotAudioHandler*);

int lingot_audio_start(LingotAudioHandler*);
void lingot_audio_stop(LingotAudioHandler*);

#endif
