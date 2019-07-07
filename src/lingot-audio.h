/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2019  Iban Cereijo.
 * Copyright (C) 2004-2008  Jairo Chapela.

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

#ifndef LINGOT_AUDIO_H
#define LINGOT_AUDIO_H

#include "lingot-config.h"

#include <pthread.h>

#define FLT_SAMPLE_SCALE	32767.0

// for audio systems that are self-driven (e.g. Jack), we need a callback function (and signature)
// to process the audio
typedef void (*LingotAudioProcessCallback)(FLT* read_buffer,
                                           unsigned int read_buffer_size_samples, void *arg);

typedef struct {

    int audio_system;
    char device[256];

    LingotAudioProcessCallback process_callback;
    void* process_callback_arg;

    void* audio_handler_extra;

    unsigned int read_buffer_size_samples;
    FLT* flt_read_buffer;

    unsigned int real_sample_rate;

    short bytes_per_sample;

    // pthread-related  member variables
    pthread_t thread_input_read;
    pthread_attr_t thread_input_read_attr;
    pthread_cond_t thread_input_read_cond;
    pthread_mutex_t thread_input_read_mutex;

    // indicates whether the audio thread is running
    int running;

    // indicates whether the thread was interrupted (by the audio server, not
    // by the user)
    int interrupted;
} LingotAudioHandler;

typedef struct {

    int forced_sample_rate; // tells whether the sample rate can be changed

    int n_sample_rates; // number of available sample rates
    int sample_rates [5]; // sample rates

    int n_devices; // number of available devices
    char** devices; // devices
} LingotAudioSystemProperties;


// set of callbacks that define the interaction with an audio system.
typedef void (*lingot_audio_new_t) (LingotAudioHandler* audio, const char* device, int sample_rate);
typedef void (*lingot_audio_destroy_t) (LingotAudioHandler* audio);
typedef int  (*lingot_audio_start_t) (LingotAudioHandler* audio);
typedef void (*lingot_audio_stop_t) (LingotAudioHandler* audio);
typedef void (*lingot_audio_cancel_t) (LingotAudioHandler* audio);
typedef int  (*lingot_audio_read_t) (LingotAudioHandler* audio);
typedef int  (*lingot_audio_get_audio_system_properties_t) (LingotAudioSystemProperties* properties);

// registers an audio system
int lingot_audio_system_register(const char* audio_system_name,
                                 lingot_audio_new_t func_new,
                                 lingot_audio_destroy_t func_destroy,
                                 lingot_audio_start_t func_start,
                                 lingot_audio_stop_t func_stop,
                                 lingot_audio_cancel_t func_cancel,
                                 lingot_audio_read_t func_read,
                                 lingot_audio_get_audio_system_properties_t func_system_properties);
int lingot_audio_system_find_by_name(const char* audio_system_name);
const char* lingot_audio_system_get_name(int index);
int lingot_audio_system_get_count(void);
int lingot_audio_system_get_properties(LingotAudioSystemProperties*,
                                       int audio_system_index);
// Return status : 0 for OK, else -1.

void lingot_audio_system_properties_destroy(LingotAudioSystemProperties*);

// creates an audio handler
void lingot_audio_new(LingotAudioHandler*,
                      int audio_system_index,
                      const char* device,
                      int sample_rate,
                      LingotAudioProcessCallback process_callback,
                      void *process_callback_arg);
// In case of failure, audio_system is set to -1 in the LingotAudioHandler struct.

// destroys an audio handler
void lingot_audio_destroy(LingotAudioHandler*);

int lingot_audio_start(LingotAudioHandler*);
void lingot_audio_stop(LingotAudioHandler*);

#endif
