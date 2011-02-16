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
#include "lingot-audio-jack.h"
#include "lingot-i18n.h"
#include "lingot-error.h"

#ifdef JACK
#include <jack/jack.h>

jack_client_t* client = NULL;

pthread_mutex_t stop_mutex = PTHREAD_MUTEX_INITIALIZER;

int lingot_audio_jack_process(jack_nframes_t nframes, void* param) {
	LingotAudioHandler* audio = param;
	audio->nframes = nframes;

	pthread_mutex_lock(&stop_mutex);
	if (audio->running) {
		lingot_audio_jack_read(audio);
		audio->process_callback(audio->flt_read_buffer,
				audio->read_buffer_size, audio->process_callback_arg);
	}
	pthread_mutex_unlock(&stop_mutex);

	return 0;
}

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void lingot_audio_jack_shutdown(void* param) {
	LingotAudioHandler* audio = param;
	lingot_error_queue_push(_("Missing connection with JACK audio server"));
	pthread_mutex_lock(&stop_mutex);
	// cleans the read buffer
	memset(audio->flt_read_buffer, 0, audio->read_buffer_size * sizeof(FLT));
	audio->process_callback(audio->flt_read_buffer, audio->read_buffer_size,
			audio->process_callback_arg);
	pthread_mutex_unlock(&stop_mutex);
	audio->shutdown_callback(audio->shutdown_callback_arg);
}
#endif

// TODO: quitar core
LingotAudioHandler* lingot_audio_jack_new(char* device, int sample_rate,
		LingotAudioProcessCallback process_callback,
		void *process_callback_arg,
		LingotAudioShutdownCallback shutdown_callback,
		void* shutdown_callback_arg) {

	LingotAudioHandler* audio = NULL;
	const char* exception;

	//LingotCore* core = (LingotCore*) p;
#	ifdef JACK
	const char **ports = NULL;
	const char *client_name = "lingot";
	const char *server_name = NULL;

	jack_options_t options = JackNoStartServer;
	jack_status_t status;

	audio = malloc(sizeof(LingotAudioHandler));
	strcpy(audio->device, "");

	audio->audio_system = AUDIO_SYSTEM_JACK;
	audio->jack_client = jack_client_open(client_name, options, &status,
			server_name);
	audio->running = 0;

	try {
		if (audio->jack_client == NULL) {
			throw(_("Unable to connect to JACK server"));
		}

		if (status & JackServerStarted) {
			fprintf(stderr, "JACK server started\n");
		}
		if (status & JackNameNotUnique) {
			client_name = jack_get_client_name(audio->jack_client);
			fprintf(stderr, "unique name `%s' assigned\n", client_name);
		}

		audio->process_callback = process_callback;
		audio->process_callback_arg = process_callback_arg;

		jack_on_shutdown(audio->jack_client, lingot_audio_jack_shutdown, audio);

		audio->real_sample_rate = jack_get_sample_rate(audio->jack_client);
		audio->read_buffer_size = jack_get_buffer_size(audio->jack_client);

		//	printf("engine sample rate: %" PRIu32 "\n", jack_get_sample_rate(
		//			audio->jack_client));
		//	printf("buffer size: %" PRIu32 "\n", jack_get_buffer_size(
		//			audio->jack_client));

		audio->jack_input_port = jack_port_register(audio->jack_client,
				"input", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);

		if ((audio->jack_input_port == NULL)) {
			throw(_("No more JACK ports available"));
		}

		//printf("Connected to port %s", ports[0]);

	} catch {
		free(audio);
		audio = NULL;
		lingot_error_queue_push(exception);
	}

	if (ports != NULL)
		jack_free(ports);

	if (audio != NULL) {
		client = audio->jack_client;
	}

#	else
	lingot_error_queue_push(
			_("The application has not been built with JACK support"));
#	endif
	return audio;
}

void lingot_audio_jack_destroy(LingotAudioHandler* audio) {
#	ifdef JACK
	if (audio != NULL) {
		//jack_cycle_wait(audio->jack_client);
		//		jack_deactivate(audio->jack_client);
		jack_client_close(audio->jack_client);
		client = NULL;
	}
#	endif
}

int lingot_audio_jack_read(LingotAudioHandler* audio) {
#	ifdef JACK
	register int i;
	float* in = jack_port_get_buffer(audio->jack_input_port, audio->nframes);
	for (i = 0; i < audio->nframes; i++)
		audio->flt_read_buffer[i] = in[i] * 32768;
	return 0;
#	else
	return -1;
#	endif
}

LingotAudioSystemProperties* lingot_audio_jack_get_audio_system_properties(
		audio_system_t audio_system) {

	LingotAudioSystemProperties* properties = NULL;
#	ifdef JACK
	properties = (LingotAudioSystemProperties*) malloc(1
			* sizeof(LingotAudioSystemProperties));

	int sample_rate = -1;

	const char *client_name = "lingot-get-sample-rate";
	const char *server_name = NULL;

	jack_options_t options = JackNoStartServer;
	jack_status_t status;
	jack_client_t* jack_client = NULL;
	const char **ports = NULL;
	const char* exception;

	try {
		if (client != NULL) {
			sample_rate = jack_get_sample_rate(client);
			ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical
					| JackPortIsOutput); // TODO: ???
		} else {
			jack_client = jack_client_open(client_name, options, &status,
					server_name);
			if (jack_client == NULL) {
				throw(_("Unable to connect to JACK server"));
			}
			if (status & JackServerStarted) {
				fprintf(stderr, "JACK server started\n");
			}
			if (status & JackNameNotUnique) {
				client_name = jack_get_client_name(jack_client);
				fprintf(stderr, "unique name `%s' assigned\n", client_name);
			}

			sample_rate = jack_get_sample_rate(jack_client);

			ports = jack_get_ports(jack_client, NULL, NULL, JackPortIsPhysical
					| JackPortIsOutput); // TODO: ???

		}
	} catch {
		lingot_error_queue_push(exception);
	}

	properties->forced_sample_rate = 1;
	properties->n_devices = 0;
	properties->devices = NULL;

	if (ports != NULL) {
		int i;
		for (i = 0; ports[i] != NULL; i++) {
		}
		properties->n_devices = i;

		if (properties->n_devices != 0) {
			properties->devices = malloc(properties->n_devices * sizeof(int));
			for (i = 0; ports[i] != NULL; i++) {
				properties->devices[i] = strdup(ports[i]);
			}
		}

	}

	if (sample_rate == -1) {
		properties->n_sample_rates = 0;
		properties->sample_rates = NULL;
	} else {
		properties->n_sample_rates = 1;
		properties->sample_rates = malloc(properties->n_sample_rates
				* sizeof(int));
		properties->sample_rates[0] = sample_rate;
	}

	if (ports != NULL)
		jack_free(ports);

	if (jack_client != NULL)
		jack_client_close(jack_client);

#	else
	lingot_error_queue_push(
			_("The application has not been built with JACK support"));
#	endif

	return properties;
}

int lingot_audio_jack_start(LingotAudioHandler* audio) {
	int result = 0;
	const char **ports = NULL;
	const char* exception;

	jack_set_process_callback(audio->jack_client, lingot_audio_jack_process,
			audio);

	try {
		if (jack_activate(audio->jack_client)) {
			throw(_("Cannot activate client"));
		}

		ports = jack_get_ports(audio->jack_client, NULL, NULL,
				JackPortIsPhysical | JackPortIsOutput); // TODO: ???
		if (ports == NULL) {
			//			jack_deactivate(audio->jack_client);
			throw(_("No physical capture ports"));
		}

		if (jack_connect(audio->jack_client, ports[0], jack_port_name(
				audio->jack_input_port))) {
			//			jack_deactivate(audio->jack_client);
			throw(_("Cannot connect input ports"));
		}

		audio->running = 1;

	} catch {
		lingot_error_queue_push(exception);
		result = -1;
	}

	return result;
}

void lingot_audio_jack_stop(LingotAudioHandler* audio) {
	//jack_cycle_wait(audio->jack_client);
	pthread_mutex_lock(&stop_mutex);
	jack_deactivate(audio->jack_client);
	pthread_mutex_unlock(&stop_mutex);
}
