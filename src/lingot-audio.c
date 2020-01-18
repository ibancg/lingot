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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include "lingot-defs.h"
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
} LingotAudioSystemConnector;

static int audio_system_counter = 0;
static LingotAudioSystemConnector audio_systems[10];

int lingot_audio_system_register(const char* audio_system_name,
                                 lingot_audio_new_t func_new,
                                 lingot_audio_destroy_t func_destroy,
                                 lingot_audio_start_t func_start,
                                 lingot_audio_stop_t func_stop,
                                 lingot_audio_cancel_t func_cancel,
                                 lingot_audio_read_t func_read,
                                 lingot_audio_get_audio_system_properties_t func_system_properties) {

    // printf("Found audio plugin '%s'\n", audio_system_name);

    if ((size_t) (audio_system_counter + 1) >= sizeof(audio_systems)/sizeof (LingotAudioSystemConnector)) {
        return -1;
    }
    LingotAudioSystemConnector funcs;
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

static LingotAudioSystemConnector* lingot_audio_system_get(int audio_system_index) {
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
    const LingotAudioSystemConnector* system = lingot_audio_system_get(index);
    return system ? system->audio_system_name : NULL;
}

int lingot_audio_system_get_count(void) {
    return audio_system_counter;
}

void lingot_audio_new(LingotAudioHandler* result,
                      int audio_system_index,
                      const char* device,
                      int sample_rate,
                      LingotAudioProcessCallback process_callback,
                      void *process_callback_arg) {

    result->audio_system = audio_system_index;
    LingotAudioSystemConnector* system = lingot_audio_system_get(audio_system_index);
    if (system && system->func_new) {
        system->func_new(result, device, sample_rate);

        if (result->audio_system != -1 ) {
            // audio source read in floating point format.
            result->flt_read_buffer = malloc(
                        result->read_buffer_size_samples * sizeof(FLT));
            memset(result->flt_read_buffer, 0,
                   result->read_buffer_size_samples * sizeof(FLT));
            result->process_callback = process_callback;
            result->process_callback_arg = process_callback_arg;
            result->interrupted = 0;
            result->running = 0;
        }
    }
}

void lingot_audio_destroy(LingotAudioHandler* audio) {

    LingotAudioSystemConnector* system = lingot_audio_system_get(audio->audio_system);
    if (system && system->func_destroy) {
        system->func_destroy(audio);
    }

    free(audio->flt_read_buffer);
    audio->flt_read_buffer = NULL;
    audio->audio_system = -1;
}

int lingot_audio_read(LingotAudioHandler* audio) {
    int samples_read = -1;

    LingotAudioSystemConnector* system = lingot_audio_system_get(audio->audio_system);
    if (system && system->func_read) {
        samples_read = system->func_read(audio);

        //#		define RATE_ESTIMATOR

#		ifdef RATE_ESTIMATOR
        static double samplerate_estimator = 0.0;
        static unsigned long read_samples = 0;
        static double elapsed_time = 0.0;

        struct timeval tdiff, t_abs;
        static struct timeval t_abs_old = { .tv_sec = 0, .tv_usec = 0 };
        static FILE* fid = 0x0;

        if (fid == 0x0) {
            fid = fopen("/tmp/dump.txt", "w");
        }

        gettimeofday(&t_abs, NULL );

        if ((t_abs_old.tv_sec != 0) || (t_abs_old.tv_usec != 0)) {

            int i;
            for (i = 0; i < samples_read; i++) {
                fprintf(fid, "%f ", audio->flt_read_buffer[i]);
                //				printf("%f ", audio->flt_read_buffer[i]);
            }
            //			printf("\n");
            timersub(&t_abs, &t_abs_old, &tdiff);
            read_samples = samples_read;
            elapsed_time = tdiff.tv_sec + 1e-6 * tdiff.tv_usec;
            static const double c = 0.9;
            samplerate_estimator = c * samplerate_estimator
                    + (1 - c) * (read_samples / elapsed_time);
            //			printf("estimated sample rate %f (read %i samples in %f seconds)\n",
            //					samplerate_estimator, read_samples, elapsed_time);

        }
        t_abs_old = t_abs;
#		endif
    }

    return samples_read;
}

int lingot_audio_system_get_properties(LingotAudioSystemProperties* properties,
                                       int audio_system_index) {

    LingotAudioSystemConnector* system = lingot_audio_system_get(audio_system_index);
    if (system && system->func_system_properties) {
        return system->func_system_properties(properties);
    }

    return -1;
}

void lingot_audio_system_properties_destroy(
        LingotAudioSystemProperties* properties) {

    int i;
    if (properties->devices) {
        for (i = 0; i < properties->n_devices; i++) {
            if (properties->devices[i]) {
                free((void*) properties->devices[i]);
            }
        }
        free(properties->devices);
    }
}

void* lingot_audio_mainloop(void* _audio) {

    int samples_read = 0;
    LingotAudioHandler* audio = _audio;

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

int lingot_audio_start(LingotAudioHandler* audio) {

    int result = -1;

    LingotAudioSystemConnector* system = lingot_audio_system_get(audio->audio_system);
    if (system) {
        if (system->func_start) {
            result = system->func_start(audio);
        } else {
            pthread_attr_init(&audio->thread_input_read_attr);

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

    if (result == 0) {
        audio->running = 1;
    }

    return result;
}

// function invoked when the audio thread must be cancelled
void lingot_audio_cancel(LingotAudioHandler* audio) {
    // TODO: avoid
    fprintf(stderr, "warning: canceling audio thread\n");

    LingotAudioSystemConnector* system = lingot_audio_system_get(audio->audio_system);
    if (system && system->func_cancel) {
        system->func_cancel(audio);
    }
}

void lingot_audio_stop(LingotAudioHandler* audio) {
    void* thread_result;

    int result;
    struct timeval tout_abs;
    struct timespec tout_tspec;

    gettimeofday(&tout_abs, NULL );

    if (audio->running == 1) {
        audio->running = 0;
        LingotAudioSystemConnector* system = lingot_audio_system_get(audio->audio_system);
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
