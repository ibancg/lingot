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

#ifndef LINGOT_IO_CONFIG_H
#define LINGOT_IO_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lingot-config.h"
#include "lingot-io-config-scale.h"

extern char CONFIG_FILE_NAME[200];

#define LINGOT_CONFIG_DIR_NAME             ".config/lingot/"
#define LINGOT_DEFAULT_CONFIG_FILE_NAME    "lingot.conf"

// configuration parameter identifier
typedef enum {
    LINGOT_PARAMETER_ID_AUDIO_SYSTEM, //
    LINGOT_PARAMETER_ID_ROOT_FREQUENCY_ERROR, //
    LINGOT_PARAMETER_ID_FFT_SIZE, //
    LINGOT_PARAMETER_ID_TEMPORAL_WINDOW, //
    LINGOT_PARAMETER_ID_MIN_SNR, //
    LINGOT_PARAMETER_ID_CALCULATION_RATE, //
    LINGOT_PARAMETER_ID_VISUALIZATION_RATE, //
    LINGOT_PARAMETER_ID_MINIMUM_FREQUENCY, //
    LINGOT_PARAMETER_ID_MAXIMUM_FREQUENCY, //
    // ------- obsolete ---------
    LINGOT_PARAMETER_ID_MIN_FREQUENCY, //
    LINGOT_PARAMETER_ID_GAIN, //
    LINGOT_PARAMETER_ID_NOISE_THRESHOLD, //
    LINGOT_PARAMETER_ID_SAMPLE_RATE, //
    LINGOT_PARAMETER_ID_OVERSAMPLING, //
    LINGOT_PARAMETER_ID_DFT_NUMBER, //
    LINGOT_PARAMETER_ID_DFT_SIZE, //
    LINGOT_PARAMETER_ID_PEAK_ORDER, //
    LINGOT_PARAMETER_ID_PEAK_NUMBER, //
    LINGOT_PARAMETER_ID_PEAK_HALF_WIDTH, //
    LINGOT_PARAMETER_ID_PEAK_REJECTION_RELATION, //

    LINGOT_PARAMETER_ID_AUDIO_DEV, //
    LINGOT_PARAMETER_ID_AUDIO_DEV_ALSA, //
    LINGOT_PARAMETER_ID_AUDIO_DEV_JACK, //
    LINGOT_PARAMETER_ID_AUDIO_DEV_PULSEAUDIO, //
} lingot_config_parameter_id_t;

// configuration parameter type
typedef enum {
    LINGOT_PARAMETER_TYPE_STRING,
    LINGOT_PARAMETER_TYPE_INTEGER,
    LINGOT_PARAMETER_TYPE_FLOAT,
    LINGOT_PARAMETER_TYPE_AUDIO_SYSTEM
} lingot_config_parameter_type_t;

// configuration parameter specification (id, type, minimum and maximum allowed values, ...)
typedef struct {

    lingot_config_parameter_id_t id;
    lingot_config_parameter_type_t type;
    const char* name;
    const char* units;

    int deprecated;

    unsigned int str_max_len;
    int int_min;
    int int_max;
    double float_min;
    double float_max;

} lingot_config_parameter_spec_t;

void lingot_io_config_create_parameter_specs(void);
lingot_config_parameter_spec_t lingot_io_config_get_parameter_spec(lingot_config_parameter_id_t id);

void lingot_io_config_save(lingot_config_t*, const char* filename);
int lingot_io_config_load(lingot_config_t*, const char* filename);

#ifdef __cplusplus
}
#endif

#endif // LINGOT_IO_CONFIG_H
