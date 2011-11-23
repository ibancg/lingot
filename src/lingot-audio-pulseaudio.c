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

#include <stdio.h>

#include "lingot-defs.h"
#include "lingot-audio-pulseaudio.h"
#include "lingot-i18n.h"
#include "lingot-msg.h"

#ifdef PULSEAUDIO
#include <pulse/pulseaudio.h>
#endif

LingotAudioHandler* lingot_audio_pulseaudio_new(char* device, int sample_rate) {

	LingotAudioHandler* audio = NULL;

#	ifdef PULSEAUDIO

	audio = malloc(sizeof(LingotAudioHandler));
	audio->pa_client = 0;
	strcpy(audio->device, "");
	audio->read_buffer = NULL;
	audio->audio_system = AUDIO_SYSTEM_PULSEAUDIO;
	audio->read_buffer_size = 128; // TODO: size up

	int error;

	pa_sample_spec ss;
	ss.format = PA_SAMPLE_S16LE;
	ss.channels = 1;
	ss.rate = sample_rate;

	pa_buffer_attr buff;
	const unsigned int iBlockLen = audio->read_buffer_size;
//		buff.tlength = iBlockLen;
//		buff.minreq = iBlockLen;
	buff.maxlength = -1;
//		buff.prebuf = -1;
	buff.fragsize = iBlockLen;

	audio->pa_client = pa_simple_new(NULL, // Use the default server.
			"Lingot", // Our application's name.
			PA_STREAM_RECORD, NULL, // Use the default device.
			"record", // Description of our stream.
			&ss, // Our sample format.
			NULL, // Use default channel map
			&buff, // Use default buffering attributes.
			&error // Ignore error code.
			);

	if (!audio->pa_client) {
		char buff[100];
		sprintf(buff, _("Error creating PulseAudio client.\n%s."),
				pa_strerror(error));
		lingot_msg_add_error(buff);
		free(audio);
		audio = NULL;
	} else {
		audio->real_sample_rate = sample_rate;
		audio->read_buffer = malloc(
				ss.channels * audio->read_buffer_size * sizeof(SAMPLE_TYPE));
		memset(audio->read_buffer, 0,
				audio->read_buffer_size * sizeof(SAMPLE_TYPE));
	}

#	else
	lingot_msg_add_error(
			_("The application has not been built with PulseAudio support"));
#	endif

	return audio;
}

void lingot_audio_pulseaudio_destroy(LingotAudioHandler* audio) {

#	ifdef PULSEAUDIO
	if (audio != NULL) {
		if (audio->pa_client != 0) {
			pa_simple_free(audio->pa_client);
			audio->pa_client = 0x0;
			free(audio->read_buffer);
		}
	}
#	endif
}

int lingot_audio_pulseaudio_read(LingotAudioHandler* audio) {

	int samples_read = -1;
#	ifdef PULSEAUDIO
	int error;

	if (pa_simple_read(audio->pa_client, audio->read_buffer,
			audio->read_buffer_size * sizeof(SAMPLE_TYPE), &error) < 0) {
		char buff[100];
		sprintf(buff, _("Read from audio interface failed.\n%s."),
				pa_strerror(error));
		lingot_msg_add_error(buff);
	} else {
		samples_read = audio->read_buffer_size;
		int i;
		for (i = 0; i < audio->read_buffer_size; i++) {
			audio->flt_read_buffer[i] = audio->read_buffer[i];
		}
	}

#	endif

	return samples_read;
}

LingotAudioSystemProperties* lingot_audio_pulseaudio_get_audio_system_properties(
		audio_system_t audio_system) {

	LingotAudioSystemProperties* properties = NULL;
#	ifdef PULSEAUDIO
	properties = (LingotAudioSystemProperties*) malloc(
			1 * sizeof(LingotAudioSystemProperties));

	properties->forced_sample_rate = 0;
	properties->n_devices = 0;
	properties->devices = NULL;
	properties->n_sample_rates = 5;
	properties->sample_rates = malloc(properties->n_sample_rates * sizeof(int));
	properties->sample_rates[0] = 8000;
	properties->sample_rates[1] = 11025;
	properties->sample_rates[2] = 22050;
	properties->sample_rates[3] = 44100;
	properties->sample_rates[4] = 48000;

#	endif

	return properties;
}

