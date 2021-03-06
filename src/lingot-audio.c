/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2020  Iban Cereijo.
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
 * along with lingot; if not, write to the Free Software Foundation,
 * Inc. 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include "lingot-defs-internal.h"
#include "lingot-audio.h"

#include "lingot-core.h"
#include "lingot-i18n.h"
#include "lingot-msg.h"

// set of callbacks that define the interaction with an audio system.
typedef struct {
    //    int audio_system_index;
    const char* audio_system_name;
    lingot_audio_new_t func_new;
    lingot_audio_destroy_t func_destroy;
    lingot_audio_start_t func_start;
    lingot_audio_stop_t func_stop;
    lingot_audio_cancel_t func_cancel;
    lingot_audio_read_t func_read;
    lingot_audio_get_audio_system_properties_t func_system_properties;
} lingot_audio_system_connector_t;

static int audio_system_counter = 0;
static lingot_audio_system_connector_t audio_systems[10];

int lingot_audio_system_register(const char* audio_system_name,
                                 lingot_audio_new_t func_new,
                                 lingot_audio_destroy_t func_destroy,
                                 lingot_audio_start_t func_start,
                                 lingot_audio_stop_t func_stop,
                                 lingot_audio_cancel_t func_cancel,
                                 lingot_audio_read_t func_read,
                                 lingot_audio_get_audio_system_properties_t func_system_properties) {

    // printf("Found audio plugin '%s'\n", audio_system_name);

    if ((size_t) (audio_system_counter + 1) >= sizeof(audio_systems)/sizeof (lingot_audio_system_connector_t)) {
        return -1;
    }
    lingot_audio_system_connector_t funcs;
    //    funcs.audio_system_index = audio_system_counter;
    funcs.audio_system_name = audio_system_name;
    funcs.func_new = func_new;
    funcs.func_destroy = func_destroy;
    funcs.func_start = func_start;
    funcs.func_stop = func_stop;
    funcs.func_cancel = func_cancel;
    funcs.func_read = func_read;
    funcs.func_system_properties = func_system_properties;
    audio_systems[audio_system_counter] = funcs;
    return audio_system_counter++;
}

static lingot_audio_system_connector_t* lingot_audio_system_get(int audio_system_index) {
    return ((audio_system_index >= 0) && (audio_system_index < audio_system_counter)) ?
                &audio_systems[audio_system_index] : NULL;
}


int lingot_audio_system_find_by_name(const char *audio_system_name)
{
    int index;
    for (index = 0; index < audio_system_counter; ++index) {
        if (!strcmp(audio_systems[index].audio_system_name, audio_system_name)) {
            return index;
        }
    }
    return -1;
}

const char* lingot_audio_system_get_name(int index) {
    const lingot_audio_system_connector_t* system = lingot_audio_system_get(index);
    return system ? system->audio_system_name : NULL;
}

int lingot_audio_system_get_count(void) {
    return audio_system_counter;
}

void lingot_audio_new(lingot_audio_handler_t* result,
                      int audio_system_index,
                      const char* device,
                      int sample_rate,
                      lingot_audio_process_callback_t process_callback,
                      void *process_callback_arg) {

    result->audio_system = audio_system_index;
    lingot_audio_system_connector_t* system = lingot_audio_system_get(audio_system_index);
    if (system && system->func_new) {
        system->func_new(result, device, sample_rate);

        if (result->audio_system != -1 ) {
            // audio source read in floating point format.
            result->flt_read_buffer = malloc(result->read_buffer_size_samples * sizeof(LINGOT_FLT));
            memset(result->flt_read_buffer, 0, result->read_buffer_size_samples * sizeof(LINGOT_FLT));
            result->process_callback = process_callback;
            result->process_callback_arg = process_callback_arg;
            result->interrupted = 0;
            result->running = 0;
        }
    }
}

void lingot_audio_destroy(lingot_audio_handler_t* audio) {

    lingot_audio_system_connector_t* system = lingot_audio_system_get(audio->audio_system);
    if (system && system->func_destroy) {
        system->func_destroy(audio);
    }

    free(audio->flt_read_buffer);
    audio->flt_read_buffer = NULL;
    audio->audio_system = -1;
}

int lingot_audio_read(lingot_audio_handler_t* audio) {
    int samples_read = -1;

    lingot_audio_system_connector_t* system = lingot_audio_system_get(audio->audio_system);
    if (system && system->func_read) {
        samples_read = system->func_read(audio);
    }

    return samples_read;
}

int lingot_audio_system_get_properties(lingot_audio_system_properties_t* properties,
                                       int audio_system_index) {

    lingot_audio_system_connector_t* system = lingot_audio_system_get(audio_system_index);
    if (system && system->func_system_properties) {
        return system->func_system_properties(properties);
    }

    return -1;
}

void lingot_audio_system_properties_destroy(lingot_audio_system_properties_t* properties) {

    int i;
    if (properties->devices) {
        for (i = 0; i < properties->n_devices; i++) {
            if (properties->devices[i]) {
                free((void*) properties->devices[i]);
            }
        }
        free(properties->devices);
        properties->devices = NULL;
    }
}

void* lingot_audio_mainloop(void* _audio) {

    int samples_read = 0;
    lingot_audio_handler_t* audio = _audio;

    while (audio->running) {

        // blocking read
        samples_read = lingot_audio_read(audio);

        if (samples_read < 0) {
            audio->running = 0;
            audio->interrupted = 1;
        } else {
            // process new data block.
            audio->process_callback(audio->flt_read_buffer,
                                    (unsigned int) samples_read,
                                    audio->process_callback_arg);
        }
    }

    pthread_mutex_lock(&audio->thread_input_read_mutex);
    pthread_cond_broadcast(&audio->thread_input_read_cond);
    pthread_mutex_unlock(&audio->thread_input_read_mutex);

    return NULL;
}

int lingot_audio_start(lingot_audio_handler_t* audio) {

    int result = -1;

    lingot_audio_system_connector_t* system = lingot_audio_system_get(audio->audio_system);
    if (system) {
        if (system->func_start) {
            result = system->func_start(audio);
        } else {
            pthread_attr_init(&audio->thread_input_read_attr);

            audio->running = 1;

            // detached thread.
            //  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            pthread_mutex_init(&audio->thread_input_read_mutex, NULL );
            pthread_cond_init(&audio->thread_input_read_cond, NULL );
            pthread_attr_init(&audio->thread_input_read_attr);
            pthread_create(&audio->thread_input_read,
                           &audio->thread_input_read_attr,
                           lingot_audio_mainloop, audio);
            result = 0;
        }
    }

    return result;
}

// function invoked when the audio thread must be cancelled
void lingot_audio_cancel(lingot_audio_handler_t* audio) {
    // TODO: avoid
    fprintf(stderr, "warning: canceling audio thread\n");

    lingot_audio_system_connector_t* system = lingot_audio_system_get(audio->audio_system);
    if (system && system->func_cancel) {
        system->func_cancel(audio);
    }
}

void lingot_audio_stop(lingot_audio_handler_t* audio) {
    void* thread_result;

    int result;
    struct timeval tout_abs;
    struct timespec tout_tspec;

    gettimeofday(&tout_abs, NULL );

    if (audio->running) {
        audio->running = 0;
        lingot_audio_system_connector_t* system = lingot_audio_system_get(audio->audio_system);
        if (system) {
            if (system->func_stop) {
                system->func_stop(audio);
            } else {
                tout_abs.tv_usec += 500000;
                if (tout_abs.tv_usec >= 1000000) {
                    tout_abs.tv_usec -= 1000000;
                    tout_abs.tv_sec++;
                }
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
            }
        }
    }
}
