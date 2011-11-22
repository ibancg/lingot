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

#include <stdlib.h>

#include "lingot-defs.h"
#include "lingot-audio-alsa.h"
#include "lingot-i18n.h"
#include "lingot-msg.h"

LingotAudioHandler* lingot_audio_alsa_new(char* device, int sample_rate) {

	LingotAudioHandler* audio = NULL;

#	ifdef ALSA
	const char* exception;
	snd_pcm_hw_params_t* hw_params = NULL;
	int err;
	char error_message[1000];
	unsigned int channels = 1;

	audio = malloc(sizeof(LingotAudioHandler));
	audio->read_buffer = NULL;
	audio->audio_system = AUDIO_SYSTEM_ALSA;
	audio->read_buffer_size = 128; // TODO: size up

	// ALSA allocates some mem to load its config file when we call
	// snd_card_next. Now that we're done getting the info, let's tell ALSA
	// to unload the info and free up that mem
	snd_config_update_free_global();

	audio->capture_handle = NULL;

	try {
		if ((err = snd_pcm_open(&audio->capture_handle, device,
				SND_PCM_STREAM_CAPTURE, 0)) < 0) {
			sprintf(error_message, "Cannot open audio device %s.\n%s.", device,
					snd_strerror(err));
			throw(error_message);
		}

		strcpy(audio->device, device);

		if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
			sprintf(error_message,
					"Cannot allocate hardware parameter structure.\n%s.",
					snd_strerror(err));
			throw(error_message);
		}

		if ((err = snd_pcm_hw_params_any(audio->capture_handle, hw_params))
				< 0) {
			sprintf(error_message,
					"Cannot initialize hardware parameter structure.\n%s.",
					snd_strerror(err));
			throw(error_message);
		}

		if ((err = snd_pcm_hw_params_set_access(audio->capture_handle,
				hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
			sprintf(error_message, "Cannot set access type.\n%s",
					snd_strerror(err));
			throw(error_message);
		}

		if ((err = snd_pcm_hw_params_set_format(audio->capture_handle,
				hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
			sprintf(error_message, "Cannot set sample format.\n%s.",
					snd_strerror(err));
			throw(error_message);
		}

		unsigned int rate = sample_rate;

		if ((err = snd_pcm_hw_params_set_rate_near(audio->capture_handle,
				hw_params, &rate, 0)) < 0) {
			sprintf(error_message, "Cannot set sample rate.\n%s.",
					snd_strerror(err));
			throw(error_message);
		}

		audio->real_sample_rate = rate;

		if ((err = snd_pcm_hw_params_set_channels(audio->capture_handle,
				hw_params, channels)) < 0) {
			sprintf(error_message, "Cannot set channel count.\n%s.",
					snd_strerror(err));
			throw(error_message);
		}

		if ((err = snd_pcm_hw_params(audio->capture_handle, hw_params)) < 0) {
			sprintf(error_message, "Cannot set parameters.\n%s.",
					snd_strerror(err));
			throw(error_message);
		}

		if ((err = snd_pcm_prepare(audio->capture_handle)) < 0) {
			sprintf(error_message,
					"Cannot prepare audio interface for use.\n%s.",
					snd_strerror(err));
			throw(error_message);
		}

		audio->read_buffer = malloc(
				channels * audio->read_buffer_size * sizeof(SAMPLE_TYPE));
		memset(audio->read_buffer, 0,
				audio->read_buffer_size * sizeof(SAMPLE_TYPE));
	}catch {
		if (audio->capture_handle != NULL)
			snd_pcm_close(audio->capture_handle);
		free(audio);
		audio = NULL;
		lingot_msg_add_error(exception);
	}

	if (hw_params != NULL)
		snd_pcm_hw_params_free(hw_params);

#	else
	lingot_msg_add_error(
			_("The application has not been built with ALSA support"));
#	endif

	return audio;
}

void lingot_audio_alsa_destroy(LingotAudioHandler* audio) {
#	ifdef ALSA
	if (audio != NULL) {
		snd_pcm_close(audio->capture_handle);
		free(audio->read_buffer);
	}
#	endif
}

int lingot_audio_alsa_read(LingotAudioHandler* audio) {
#	ifdef ALSA
	int samples_read = snd_pcm_readi(audio->capture_handle, audio->read_buffer,
			audio->read_buffer_size);

	if (samples_read < 0) {
		char buff[100];
		sprintf(buff, _("Read from audio interface failed.\n%s."),
				snd_strerror(samples_read));
		lingot_msg_add_error(buff);
	} else {
		int i;
		// float point conversion
		for (i = 0; i < samples_read; i++) {
			audio->flt_read_buffer[i] = audio->read_buffer[i];
		}
	}

#	endif

	return samples_read;
}

LingotAudioSystemProperties* lingot_audio_alsa_get_audio_system_properties(
		audio_system_t audio_system) {

	LingotAudioSystemProperties* result = (LingotAudioSystemProperties*) malloc(
			1 * sizeof(LingotAudioSystemProperties));

	result->forced_sample_rate = 0;
	result->n_devices = 0;
	result->devices = NULL;

	result->n_sample_rates = 5;
	result->sample_rates = malloc(result->n_sample_rates * sizeof(int));
	result->sample_rates[0] = 8000;
	result->sample_rates[1] = 11025;
	result->sample_rates[2] = 22050;
	result->sample_rates[3] = 44100;
	result->sample_rates[4] = 48000;

#	ifdef ALSA

	int status;
	int card_index = -1;
	char* card_longname = NULL;
	char* card_shortname = NULL;
	const char* exception;
	char error_message[1000];
	char device_name[100];
	char str[64];
	int device_index = -1;
	int subdevice_count, subdevice_index;
	snd_ctl_t *card_handler;
	snd_pcm_info_t *pcm_info;

	// linked list struct for storing the capture device names
	struct device_name_node_t {
		char* name;
		struct device_name_node_t* next;
	};

	struct device_name_node_t* device_names_first = NULL;
	struct device_name_node_t* device_names_last = NULL;

	try
	{
		result->n_devices = 0;
		try {
			for (;;) {

				if ((status = snd_card_next(&card_index)) < 0) {
					sprintf(error_message,
							"warning: cannot determine card number: %s",
							snd_strerror(status));
					throw(error_message);
				}

				if (card_index < 0) {
					if (result->n_devices == 0) {
						sprintf(error_message,
								"warning: no sound cards detected");
						throw(error_message);
					}
					break;
				}

				if ((status = snd_card_get_name(card_index, &card_shortname))
						< 0) {
					sprintf(error_message,
							"warning: cannot determine card shortname: %s",
							snd_strerror(status));
					throw(error_message);
				}

				sprintf(device_name, "hw:%i", card_index);
				if ((status = snd_ctl_open(&card_handler, device_name, 0))
						< 0) {
					sprintf(error_message, "warning: can't open card %i: %s\n",
							card_index, snd_strerror(status));
					throw(error_message);
				}

				for (device_index = -1;;) {

					if ((status = snd_ctl_pcm_next_device(card_handler,
							&device_index)) < 0) {
						sprintf(error_message,
								"warning: can't get next PCM device: %s\n",
								snd_strerror(status));
						throw(error_message);
					}

					if (device_index < 0)
						break;

					snd_pcm_info_malloc(&pcm_info);
					memset(pcm_info, 0, snd_pcm_info_sizeof());

					snd_pcm_info_set_device(pcm_info, device_index);

					// only search for capture devices
					snd_pcm_info_set_stream(pcm_info, SND_PCM_STREAM_CAPTURE);

					subdevice_index = -1;
					subdevice_count = 1;

					while (++subdevice_index < subdevice_count) {

						snd_pcm_info_set_subdevice(pcm_info, subdevice_index);
						if ((status = snd_ctl_pcm_info(card_handler, pcm_info))
								< 0) {
							sprintf(
									error_message,
									"warning: can't get info for subdevice hw:%i,%i,%i: %s\n",
									card_index, device_index, subdevice_index,
									snd_strerror(status));
							perror(error_message);
							continue;
						}

						if (!subdevice_index) {
							subdevice_count = snd_pcm_info_get_subdevices_count(
									pcm_info);
						}

						if (subdevice_count > 1) {
							sprintf(device_name, "%s <plughw:%i,%i,%i>",
									card_shortname, card_index, device_index,
									subdevice_index);
						} else {
							sprintf(device_name, "%s <plughw:%i,%i>",
									card_shortname, card_index, device_index);
						}

						result->n_devices++;
						struct device_name_node_t* new_name_node =
								(struct device_name_node_t*) malloc(
										sizeof(struct device_name_node_t*));
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
		}catch {
			result->devices = 0;
			throw(exception);
		}

		// copy the device names list
		result->devices = (char**) malloc(result->n_devices * sizeof(char*));
		struct device_name_node_t* name_node_current = device_names_first;
		for (device_index = 0; device_index < result->n_devices;
				device_index++) {
			result->devices[device_index] = name_node_current->name;
			name_node_current = name_node_current->next;
		}

	}catch {
		perror(exception);
	}

	// dispose the device names list
	struct device_name_node_t* name_node_current = device_names_first;
	struct device_name_node_t* name_node_previous = NULL;
	for (device_index = 0; device_index < result->n_devices; device_index++) {
		name_node_previous = name_node_current;
		name_node_current = name_node_current->next;
		free(name_node_previous);
	}

#	endif

	return result;
}

