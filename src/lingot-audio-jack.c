/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2009  Ibán Cereijo Graña, Jairo Chapela Martínez.
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

#ifdef JACK
#include <jack/jack.h>
#endif

int lingot_audio_jack_process(jack_nframes_t nframes, LingotCore* core) {
	jack_default_audio_sample_t *in, *out;
	core->audio->nframes = nframes;
	lingot_core_read(core);
	return 0;
}

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void lingot_audio_jack_shutdown(void *arg) {
	// TODO
}

LingotAudio* lingot_audio_jack_new(LingotCore* core) {

	const char **ports;
	const char *client_name = "lingot";
	const char *server_name = NULL;

	jack_options_t options = JackNullOption;
	jack_status_t status;

	LingotAudio* audio = malloc(sizeof(LingotAudio));
	LingotConfig* conf = core->conf;

	audio->audio_system = AUDIO_SYSTEM_JACK;
	audio->jack_client = jack_client_open(client_name, options, &status,
			server_name);
	if (audio->jack_client == NULL) {
		fprintf(stderr, "jack_client_open() failed, "
			"status = 0x%2.0x\n", status);
		if (status & JackServerFailed) {
			fprintf(stderr, "Unable to connect to JACK server\n");
		}
		exit(1);
	}
	if (status & JackServerStarted) {
		fprintf(stderr, "JACK server started\n");
	}
	if (status & JackNameNotUnique) {
		client_name = jack_get_client_name(audio->jack_client);
		fprintf(stderr, "unique name `%s' assigned\n", client_name);
	}

	jack_set_process_callback(audio->jack_client, lingot_audio_jack_process,
			core);

	jack_on_shutdown(audio->jack_client, lingot_audio_jack_shutdown, 0);

	conf->sample_rate = jack_get_sample_rate(audio->jack_client);
	lingot_config_update_internal_params(conf);
	conf->read_buffer_size = jack_get_buffer_size(audio->jack_client);

	printf("engine sample rate: %" PRIu32 "\n", jack_get_sample_rate(
			audio->jack_client));
	printf("buffer size: %" PRIu32 "\n", jack_get_buffer_size(
			audio->jack_client));

	audio->jack_input_port = jack_port_register(audio->jack_client, "input",
			JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);

	if ((audio->jack_input_port == NULL)) {
		fprintf(stderr, "no more JACK ports available\n");
		exit(1);
	}

	if (jack_activate(audio->jack_client)) {
		fprintf(stderr, "cannot activate client");
		exit(1);
	}

	ports = jack_get_ports(audio->jack_client, NULL, NULL, JackPortIsPhysical
	| JackPortIsOutput);
	if (ports == NULL) {
		fprintf(stderr, "no physical capture ports\n");
		exit(1);
	}

	if (jack_connect(audio->jack_client, ports[0], jack_port_name(
							audio->jack_input_port))) {
		fprintf(stderr, "cannot connect input ports\n");
	}

	free(ports);

	return audio;
}

void lingot_audio_jack_destroy(LingotAudio* audio) {
	jack_client_close(audio->jack_client);
}

int lingot_audio_jack_read(LingotAudio* audio, LingotCore* core) {
	register int i;
	float* in = jack_port_get_buffer(audio->jack_input_port, audio->nframes);
	for (i = 0; i < audio->nframes; i++)
		core->flt_read_buffer[i] = in[i] * 32768;
	return 0;
}
