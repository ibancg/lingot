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

#ifndef LINGOT_CONFIG_H
#define LINGOT_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lingot-defs.h"
#include "lingot-config-scale.h"

typedef enum window_type_t {
    NONE = 0, //
    HANNING = 1, //
    HAMMING = 2
} window_type_t;

#define N_MAX_AUDIO_DEV 10

// Configuration struct. Determines the behaviour of the tuner.
// Some parameters are internal only.
typedef struct {

    int audio_system_index;

    char audio_dev[N_MAX_AUDIO_DEV][512];
    int sample_rate; // hardware sample rate.
    unsigned int oversampling; // oversampling factor.

    FLT root_frequency_error; // deviation of the above root frequency.

    FLT min_frequency; // minimum frequency of the instrument.
    FLT max_frequency; // maximum frequency of the instrument.

    int optimize_internal_parameters;

    FLT internal_min_frequency; // minimum valid frequency.
    FLT internal_max_frequency; // maximum frequency we want to tune.

    unsigned int fft_size; // number of samples of the FFT.

    FLT calculation_rate;
    FLT visualization_rate;

    FLT temporal_window; // duration in seconds of the temporal window.

    // samples stored in the temporal window (internal parameter).
    unsigned int temporal_buffer_size;

    // minimum SNR required for the overall set of peaks and for each peak
    FLT min_overall_SNR; // dB
    FLT min_SNR; // dB

    window_type_t window_type;

    // frequency finding algorithm configuration
    //-------------------------------------------

    unsigned int peak_number; // number of maximum peaks considered.

    // number of adjacent samples needed to consider a peak.
    unsigned int peak_half_width;

    // max iterations for Newton-Raphson algorithm.
    unsigned int max_nr_iter;

    //----------------------------------------------------------------------------

    // global range for the gauge in cents
    FLT gauge_range;

    // gauge rest value in cents
    FLT gauge_rest_value;

    //----------------------------------------------------------------------------

    lingot_scale_t scale;

} lingot_config_t;


void lingot_config_new(lingot_config_t*);
void lingot_config_destroy(lingot_config_t*);
void lingot_config_copy(lingot_config_t* dst, const lingot_config_t* src);

// back to default configuration.
void lingot_config_restore_default_values(lingot_config_t*);

// derivate internal parameters from external ones.
void lingot_config_update_internal_params(lingot_config_t*);

#ifdef __cplusplus
}
#endif

#endif // LINGOT_CONFIG_H
