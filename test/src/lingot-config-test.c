#include <assert.h>
#include <math.h>

#include "lingot-msg.h"
#include "lingot-msg.c"
#include "lingot-config-scale.h"
#include "lingot-config-scale.c"
#include "lingot-config.h"
#include "lingot-config.c"

int lingot_config_test() {

	lingot_config_create_parameter_specs();
	LingotConfig* config = lingot_config_new();

	assert((config != NULL));

	lingot_config_load(config, "resources/lingot-001.conf");

	assert((config->audio_system == AUDIO_SYSTEM_PULSEAUDIO));
	assert(
			!strcmp(config->audio_dev[config->audio_system], "alsa_input.pci-0000_00_1b.0.analog-stereo"));
	assert((config->sample_rate == 44100));
	assert((config->oversampling == 25));
	assert((config->root_frequency_error == 0.0));
	assert((config->min_frequency == 15.0));
	assert((config->sample_rate == 44100));
	assert((config->fft_size == 512));
	assert((config->temporal_window == ((FLT) 0.32)));
	assert((config->noise_threshold_db == 20.0));
	assert((config->calculation_rate == 20.0));
	assert((config->visualization_rate == 30.0));
	assert((config->peak_number == 3));
	assert((config->peak_half_width == 1));
	assert((config->peak_rejection_relation_db == 20.0));
	assert((config->dft_number == 2));
	assert((config->dft_size == 15));
	assert(config->gain == -21.9);

	lingot_config_destroy(config);

	puts("done.");
	return 0;
}
