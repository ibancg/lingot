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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef OSS

#include "lingot-audio.h"
#include "lingot-msg.h"
#include "lingot-defs-internal.h"
#include "lingot-audio-oss.h"
#include "lingot-i18n.h"

#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int dsp; // file handler.
} lingot_audio_handler_oss_t;

void lingot_audio_oss_new(lingot_audio_handler_t* audio, const char* device, int sample_rate) {
    int channels = 1;
#	ifdef AFMT_S16_NE
    int format = AFMT_S16_NE;
#	else
    int format = AFMT_S16_LE;
#	endif

    char error_message[512];
    const char* _exception;

    audio->audio_handler_extra = malloc(sizeof(lingot_audio_handler_oss_t));
    lingot_audio_handler_oss_t* audio_oss = (lingot_audio_handler_oss_t*) audio->audio_handler_extra;

    strncpy(audio->device, device, sizeof(audio->device) - 1);
    if (!strcmp(audio->device, "default")) {
        strcpy(audio->device, "/dev/dsp");
    }
    audio_oss->dsp = open(audio->device, O_RDONLY);
    if (sample_rate >= 44100) {
        audio->read_buffer_size_samples = 1024;
    } else if (sample_rate >= 22050) {
        audio->read_buffer_size_samples = 512;
    } else {
        audio->read_buffer_size_samples = 256;
    }

    _try
    {

        if (audio_oss->dsp < 0) {
            snprintf(error_message, sizeof(error_message),
                     _("Cannot open audio device '%s'.\n%s"), audio->device,
                     strerror(errno));
            _throw(error_message);
        }

        //if (ioctl(audio->dsp, SOUND_PCM_READ_CHANNELS, &channels) < 0)
        if (ioctl(audio_oss->dsp, SNDCTL_DSP_CHANNELS, &channels) < 0) {
            snprintf(error_message, sizeof(error_message), "%s\n%s",
                     _("Error setting number of channels."), strerror(errno));
            _throw(error_message);
        }

        // sample size
        //if (ioctl(audio->dsp, SOUND_PCM_SETFMT, &format) < 0)
        if (ioctl(audio_oss->dsp, SNDCTL_DSP_SETFMT, &format) < 0) {
            snprintf(error_message, sizeof(error_message), "%s\n%s",
                     _("Error setting bits per sample."), strerror(errno));
            _throw(error_message);
        }

        int fragment_size = 1;
        int DMA_buffer_size = 512;
        int param = 0;

        for (param = 0; fragment_size < DMA_buffer_size; param++) {
            fragment_size <<= 1;
        }

        param |= 0x00ff0000;

        if (ioctl(audio_oss->dsp, SNDCTL_DSP_SETFRAGMENT, &param) < 0) {
            snprintf(error_message, sizeof(error_message), "%s\n%s",
                     _("Error setting DMA buffer size."), strerror(errno));
            _throw(error_message);
        }

        if (ioctl(audio_oss->dsp, SNDCTL_DSP_SPEED, &sample_rate) < 0) {
            snprintf(error_message, sizeof(error_message), "%s\n%s",
                     _("Error setting sample rate."), strerror(errno));
            _throw(error_message);
        }

        audio->real_sample_rate = (unsigned int) sample_rate;
        audio->bytes_per_sample = 2;
    } _catch {
        lingot_msg_add_error_with_code(_exception, errno);
        close(audio_oss->dsp);
        audio->audio_system = -1;
    }
}

void lingot_audio_oss_destroy(lingot_audio_handler_t* audio) {
    if (audio->audio_system >= 0) {
        lingot_audio_handler_oss_t* audio_oss = (lingot_audio_handler_oss_t*) audio->audio_handler_extra;
        if (audio_oss->dsp >= 0) {
            close(audio_oss->dsp);
            audio_oss->dsp = -1;
        }
        free(audio->audio_handler_extra);
        audio->audio_handler_extra = NULL;
    }
}

int lingot_audio_oss_read(lingot_audio_handler_t* audio) {
    int samples_read = -1;
    char buffer [audio->read_buffer_size_samples * (unsigned int) audio->bytes_per_sample];

    lingot_audio_handler_oss_t* audio_oss = (lingot_audio_handler_oss_t*) audio->audio_handler_extra;
    long int bytes_read = read(audio_oss->dsp, buffer, sizeof (buffer));

    if (bytes_read < 0) {
        char buff[512];
        snprintf(buff, sizeof(buff), "%s\n%s",
                 _("Read from audio interface failed."), strerror(errno));
        lingot_msg_add_error(buff);
    } else {

        samples_read = (int) (bytes_read / audio->bytes_per_sample);
        // float point conversion
        int i;
        const int16_t* read_buffer = (int16_t*) buffer;
        for (i = 0; i < samples_read; i++) {
            audio->flt_read_buffer[i] = read_buffer[i];
        }
    }
    return samples_read;
}

int lingot_audio_oss_get_audio_system_properties(lingot_audio_system_properties_t* result) {

    result->forced_sample_rate = 0;
    result->n_devices = 1;
    result->devices = (const char**) malloc((unsigned int) result->n_devices * sizeof(char*));
    result->devices[0] = _strdup("/dev/dsp");

    result->n_sample_rates = 5;
    result->sample_rates[0] = 8000;
    result->sample_rates[1] = 11025;
    result->sample_rates[2] = 22050;
    result->sample_rates[3] = 44100;
    result->sample_rates[4] = 48000;

    return 0;
}

int lingot_audio_oss_register(void)
{
    return lingot_audio_system_register("OSS",
                                        lingot_audio_oss_new,
                                        lingot_audio_oss_destroy,
                                        NULL,
                                        NULL,
                                        NULL,
                                        lingot_audio_oss_read,
                                        lingot_audio_oss_get_audio_system_properties);
}

#else

int lingot_audio_oss_register(void)
{
    return 0;
}

#endif
