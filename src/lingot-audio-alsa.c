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
 * along with lingot; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef ALSA

#include <stdlib.h>
#include <unistd.h>
#include <alsa/asoundlib.h>

#include "lingot-audio.h"
#include "lingot-defs.h"
#include "lingot-audio-alsa.h"
#include "lingot-i18n.h"
#include "lingot-msg.h"

typedef struct {
    snd_pcm_t *capture_handle;
} lingot_audio_handler_alsa_t;

static snd_pcm_format_t sample_format = SND_PCM_FORMAT_FLOAT;
static const unsigned int channels = 1;

void lingot_audio_alsa_new(lingot_audio_handler_t* audio, const char* device, int sample_rate) {

    const char* _exception;
    snd_pcm_hw_params_t* hw_params = NULL;
    int err;
    char error_message[1000];

    audio->audio_handler_extra = malloc(sizeof(lingot_audio_handler_alsa_t));
    lingot_audio_handler_alsa_t* audioALSA = (lingot_audio_handler_alsa_t*) audio->audio_handler_extra;

    if (sample_rate >= 44100) {
        audio->read_buffer_size_samples = 1024;
    } else if (sample_rate >= 22050) {
        audio->read_buffer_size_samples = 512;
    } else {
        audio->read_buffer_size_samples = 256;
    }

    // ALSA allocates some mem to load its config file when we call
    // snd_card_next. Now that we're done getting the info, let's tell ALSA
    // to unload the info and free up that mem
    snd_config_update_free_global();

    audioALSA->capture_handle = NULL;

    _try
    {
        if ((err = snd_pcm_open(&audioALSA->capture_handle, device,
                                SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK)) < 0) {
            snprintf(error_message, sizeof(error_message),
                     _("Cannot open audio device '%s'.\n%s"), device,
                     snd_strerror(err));
            _throw(error_message)
        }

        strncpy(audio->device, device, sizeof(audio->device) - 1);

        if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
            snprintf(error_message, sizeof(error_message), "%s\n%s",
                     _("Cannot initialize hardware parameter structure."),
                     snd_strerror(err));
            _throw(error_message)
        }

        if ((err = snd_pcm_hw_params_any(audioALSA->capture_handle, hw_params))
                < 0) {
            snprintf(error_message, sizeof(error_message), "%s\n%s",
                     _("Cannot initialize hardware parameter structure."),
                     snd_strerror(err));
            _throw(error_message)
        }

        if ((err = snd_pcm_hw_params_set_access(audioALSA->capture_handle,
                                                hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
            snprintf(error_message, sizeof(error_message), "%s\n%s",
                     _("Cannot set access type."), snd_strerror(err));
            _throw(error_message)
        }

        if ((err = snd_pcm_hw_params_set_format(audioALSA->capture_handle,
                                                hw_params, sample_format)) < 0) {
            snprintf(error_message, sizeof(error_message), "%s\n%s",
                     _("Cannot set sample format."), snd_strerror(err));
            _throw(error_message)
        }

        unsigned int rate = (unsigned int) sample_rate;

        if ((err = snd_pcm_hw_params_set_rate_near(audioALSA->capture_handle,
                                                   hw_params, &rate, 0)) < 0) {
            snprintf(error_message, sizeof(error_message), "%s\n%s",
                     _("Cannot set sample rate."), snd_strerror(err));
            _throw(error_message)
        }

        audio->real_sample_rate = rate;

        if ((err = snd_pcm_hw_params_set_channels(audioALSA->capture_handle,
                                                  hw_params, channels)) < 0) {
            snprintf(error_message, sizeof(error_message), "%s\n%s",
                     _("Cannot set channel number."), snd_strerror(err));
            _throw(error_message)
        }

        if ((err = snd_pcm_hw_params(audioALSA->capture_handle, hw_params)) < 0) {
            snprintf(error_message, sizeof(error_message), "%s\n%s",
                     _("Cannot set parameters."), snd_strerror(err));
            _throw(error_message)
        }

        if ((err = snd_pcm_prepare(audioALSA->capture_handle)) < 0) {
            snprintf(error_message, sizeof(error_message), "%s\n%s",
                     _("Cannot prepare audio interface for use."),
                     snd_strerror(err));
            _throw(error_message)
        }

        audio->bytes_per_sample = (short) snd_pcm_format_size(sample_format, 1);
    } _catch {
        if (audioALSA->capture_handle != NULL) {
            snd_pcm_close(audioALSA->capture_handle);
        }
        audio->audio_system = -1;
        lingot_msg_add_error_with_code(_exception, -err);
    }

    if (hw_params != NULL) {
        snd_pcm_hw_params_free(hw_params);
    }
}

void lingot_audio_alsa_destroy(lingot_audio_handler_t* audio) {
    if (audio->audio_system >= 0) {
        lingot_audio_handler_alsa_t* audioALSA = (lingot_audio_handler_alsa_t*) audio->audio_handler_extra;
        //        snd_pcm_drop(audioALSA->capture_handle);
        snd_pcm_close(audioALSA->capture_handle);
        free(audio->audio_handler_extra);
        audio->audio_handler_extra = NULL;
    }
}

int lingot_audio_alsa_read(lingot_audio_handler_t* audio) {
    int samples_read = -1;
    char buffer[channels * audio->read_buffer_size_samples * ((size_t) audio->bytes_per_sample)];

    lingot_audio_handler_alsa_t* audioALSA = (lingot_audio_handler_alsa_t*) audio->audio_handler_extra;
    samples_read = (int) snd_pcm_readi(audioALSA->capture_handle, buffer,
                                       audio->read_buffer_size_samples);

    if (samples_read == -EAGAIN) {
        usleep(200); // TODO: size up
        samples_read = 0;
    } else {
        if (samples_read < 0) {
            char buff[250];
            snprintf(buff, sizeof(buff), "%s\n%s",
                     _("Read from audio interface failed."),
                     snd_strerror(samples_read));
            lingot_msg_add_error_with_code(buff, -samples_read);
        } else {
            int i;
            // float point conversion
            switch (sample_format) {
            case SND_PCM_FORMAT_S16: {
                int16_t* read_buffer = (int16_t*) buffer;
                for (i = 0; i < samples_read; i++) {
                    audio->flt_read_buffer[i] = read_buffer[i];
                }
                break;
            }
            case SND_PCM_FORMAT_FLOAT: {
                float* read_buffer = (float*) buffer;
                for (i = 0; i < samples_read; i++) {
                    audio->flt_read_buffer[i] = read_buffer[i] * FLT_SAMPLE_SCALE;
                }
                break;
            }
            case SND_PCM_FORMAT_FLOAT64: {
                double* read_buffer = (double*) buffer;
                for (i = 0; i < samples_read; i++) {
                    audio->flt_read_buffer[i] = read_buffer[i] * FLT_SAMPLE_SCALE;
                }
                break;
            }
            default:
                break;
            }
        }
    }

    return samples_read;
}

int lingot_audio_alsa_get_audio_system_properties(
        lingot_audio_system_properties_t* result) {

    result->forced_sample_rate = 0;
    result->n_devices = 0;
    result->devices = NULL;

    result->n_sample_rates = 5;
    result->sample_rates[0] = 8000;
    result->sample_rates[1] = 11025;
    result->sample_rates[2] = 22050;
    result->sample_rates[3] = 44100;
    result->sample_rates[4] = 48000;

    int status;
    int card_index = -1;
    char* card_shortname = NULL;
    const char* _exception;
    char error_message[1000];
    char device_name[512];
    int device_index = -1;
    unsigned int subdevice_count;
    unsigned int subdevice_index;
    snd_ctl_t *card_handler;
    snd_pcm_info_t *pcm_info;
    struct device_name_node_t* name_node_current;

    // linked list struct for storing the capture device names
    struct device_name_node_t {
        const char* name;
        struct device_name_node_t* next;
    };

    struct device_name_node_t* device_names_first = (struct device_name_node_t*) malloc(sizeof(struct device_name_node_t));
    struct device_name_node_t* device_names_last = device_names_first;

    // the first record is the default device
    char buff[512];
    snprintf(buff, sizeof(buff), "%s <default>", _("Default Device"));
    device_names_first->name = strdup(buff);
    device_names_first->next = NULL;

    _try
    {
        result->n_devices = 1;
        _try
        {
            for (;;) {

                if ((status = snd_card_next(&card_index)) < 0) {
                    snprintf(error_message, sizeof(error_message),
                             "warning: cannot determine card number: %s",
                             snd_strerror(status));
                    _throw(error_message)
                }

                if (card_index < 0) {
                    if (result->n_devices == 0) {
                        snprintf(error_message, sizeof(error_message),
                                 "warning: no sound cards detected");
                        _throw(error_message)
                    }
                    break;
                }

                if ((status = snd_card_get_name(card_index, &card_shortname))
                        < 0) {
                    snprintf(error_message, sizeof(error_message),
                             "warning: cannot determine card short name: %s",
                             snd_strerror(status));
                    _throw(error_message)
                }

                sprintf(device_name, "hw:%i", card_index);
                if ((status = snd_ctl_open(&card_handler, device_name, 0))
                        < 0) {
                    snprintf(error_message, sizeof(error_message),
                             "warning: can't open card %i: %s\n", card_index,
                             snd_strerror(status));
                    _throw(error_message)
                }

                for (device_index = -1;;) {

                    if ((status = snd_ctl_pcm_next_device(card_handler,
                                                          &device_index)) < 0) {
                        snprintf(error_message, sizeof(error_message),
                                 "warning: can't get next PCM device: %s\n",
                                 snd_strerror(status));
                        _throw(error_message)
                    }

                    if (device_index < 0) {
                        break;
                    }

                    snd_pcm_info_malloc(&pcm_info);
                    memset(pcm_info, 0, snd_pcm_info_sizeof());

                    snd_pcm_info_set_device(pcm_info, (unsigned int) device_index);

                    // only search for capture devices
                    snd_pcm_info_set_stream(pcm_info, SND_PCM_STREAM_CAPTURE);

                    subdevice_index = (unsigned int) -1;
                    subdevice_count = 1;

                    while (++subdevice_index < subdevice_count) {

                        snd_pcm_info_set_subdevice(pcm_info, subdevice_index);
                        if ((status = snd_ctl_pcm_info(card_handler, pcm_info))
                                < 0) {
                            fprintf(stderr,
                                    "warning: can't get info for subdevice hw:%i,%i,%i: %s\n",
                                    card_index, device_index, subdevice_index,
                                    snd_strerror(status));
                            continue;
                        }

                        if (!subdevice_index) {
                            subdevice_count = snd_pcm_info_get_subdevices_count(pcm_info);
                        }

                        if (subdevice_count > 1) {
                            snprintf(device_name, sizeof(device_name),
                                     "%s <plughw:%i,%i,%i>", card_shortname,
                                     card_index, device_index, subdevice_index);
                        } else {
                            snprintf(device_name, sizeof(device_name),
                                     "%s <plughw:%i,%i>", card_shortname,
                                     card_index, device_index);
                        }

                        result->n_devices++;
                        struct device_name_node_t* new_name_node = (struct device_name_node_t*) malloc(sizeof(struct device_name_node_t*));
                        new_name_node->name = strdup(device_name);
                        new_name_node->next = NULL;

                        if (device_names_first == NULL) {
                            device_names_first = new_name_node;
                        } else {
                            device_names_last->next = new_name_node;
                        }
                        device_names_last = new_name_node;
                    }

                    snd_pcm_info_free(pcm_info);
                }

                snd_ctl_close(card_handler);
            }
        } _catch {
            result->n_devices = 1;
            _throw(_exception)
        }

        // copy the device names list
        result->devices = (const char**) malloc((unsigned int) result->n_devices * sizeof(char*));
        name_node_current = device_names_first;
        for (device_index = 0; device_index < result->n_devices; device_index++) {
            result->devices[device_index] = name_node_current->name;
            name_node_current->name = NULL;
            name_node_current = name_node_current->next;
        }

    } _catch {
        fprintf(stderr, "%s", _exception);
    }

    // dispose the device names list
    for (name_node_current = device_names_first; name_node_current != NULL;) {
        struct device_name_node_t* name_node_previous = name_node_current;
        name_node_current = name_node_current->next;
        free(name_node_previous);
    }
    return 0;
}

int lingot_audio_alsa_register(void)
{
    return lingot_audio_system_register("ALSA",
                                        lingot_audio_alsa_new,
                                        lingot_audio_alsa_destroy,
                                        NULL,
                                        NULL,
                                        NULL,
                                        lingot_audio_alsa_read,
                                        lingot_audio_alsa_get_audio_system_properties);
}

#else

int lingot_audio_alsa_register(void)
{
    return 0;
}

#endif
