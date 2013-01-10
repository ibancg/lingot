/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2013  Ibán Cereijo Graña, Jairo Chapela Martínez.
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
#include <math.h>
#include <sys/soundcard.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>

#ifndef LIBFFTW
#include "lingot-complex.h"
#endif

#include "lingot-fft.h"
#include "lingot-signal.h"
#include "lingot-core.h"
#include "lingot-config.h"
#include "lingot-i18n.h"
#include "lingot-msg.h"

int
lingot_core_read_callback(FLT* read_buffer, int read_buffer_size, void *arg);

void lingot_core_run_computation_thread(LingotCore* core);

int decimation_input_index = 0;

LingotCore* lingot_core_new(LingotConfig* conf) {

	char buff[1000];
	LingotCore* core = malloc(sizeof(LingotCore));

	core->conf = conf;
	core->running = 0;
	core->audio = NULL;
	core->spd_fft = NULL;
	core->noise_level = NULL;
	core->SPL = NULL;
	core->spd_dft = NULL;
	core->flt_read_buffer = NULL;
	core->temporal_buffer = NULL;
	core->windowed_temporal_buffer = NULL;
	core->windowed_fft_buffer = NULL;
	core->hamming_window_temporal = NULL;
	core->hamming_window_fft = NULL;
	core->antialiasing_filter = NULL;

	// TODO
	core->markers_size = 0;

	int requested_sample_rate = conf->sample_rate;

	if (conf->sample_rate <= 0) {
		conf->sample_rate = 0;
	}

	core->audio = lingot_audio_new(conf->audio_system,
			conf->audio_dev[conf->audio_system], conf->sample_rate,
			(LingotAudioProcessCallback) lingot_core_read_callback, core);

	if (core->audio != NULL ) {

		if (requested_sample_rate != core->audio->real_sample_rate) {
			conf->sample_rate = core->audio->real_sample_rate;
			lingot_config_update_internal_params(conf);
			sprintf(buff,
					_("The requested sample rate is not available, the real sample rate has been set to %d Hz"),
					core->audio->real_sample_rate);
			lingot_msg_add_warning(buff);
		}

		if (conf->temporal_buffer_size < conf->fft_size) {
			conf->temporal_window = ((double) conf->fft_size
					* conf->oversampling) / conf->sample_rate;
			conf->temporal_buffer_size = conf->fft_size;
			lingot_config_update_internal_params(conf);
			sprintf(buff,
					_(
							"The temporal buffer is smaller than FFT size. It has been increased to %0.3f seconds"),
					conf->temporal_window);
			lingot_msg_add_warning(buff);
		}

		// Since the SPD is symmetrical, we only store the 1st half. In the
		// particular case of only 256 samples, we store the whole SPD.
		int spd_size =
				(core->conf->fft_size > 256) ?
						(core->conf->fft_size >> 1) : core->conf->fft_size;

		core->spd_fft = malloc(spd_size * sizeof(FLT));
		core->noise_level = malloc(spd_size * sizeof(FLT));
		core->SPL = malloc(spd_size * sizeof(FLT));
		memset(core->spd_fft, 0, spd_size * sizeof(FLT));
		memset(core->noise_level, 0, spd_size * sizeof(FLT));
		memset(core->SPL, 0, spd_size * sizeof(FLT));

		core->spd_dft = malloc((core->conf->dft_size) * sizeof(FLT));
		memset(core->spd_dft, 0, core->conf->dft_size * sizeof(FLT));

//		core->diff2_spd_fft = malloc(core->conf->fft_size * sizeof(FLT)); // 2nd derivative from SPD.
//		memset(core->diff2_spd_fft, 0, core->conf->fft_size * sizeof(FLT));

		memset(core->spd_dft, 0, core->conf->dft_size * sizeof(FLT));

		// audio source read in floating point format.
		core->flt_read_buffer = malloc(
				core->audio->read_buffer_size * sizeof(FLT));
		memset(core->flt_read_buffer, 0,
				core->audio->read_buffer_size * sizeof(FLT));

		// stored samples.
		core->temporal_buffer = malloc(
				(core->conf->temporal_buffer_size) * sizeof(FLT));
		memset(core->temporal_buffer, 0,
				core->conf->temporal_buffer_size * sizeof(FLT));

		core->hamming_window_temporal = NULL;
		core->hamming_window_fft = NULL;

		if (conf->window_type != NONE) {
			core->hamming_window_temporal = malloc(
					(core->conf->temporal_buffer_size) * sizeof(FLT));
			core->hamming_window_fft = malloc(
					(core->conf->fft_size) * sizeof(FLT));

			lingot_signal_window(core->conf->temporal_buffer_size,
					core->hamming_window_temporal, conf->window_type);
			lingot_signal_window(core->conf->fft_size, core->hamming_window_fft,
					conf->window_type);
		}

		core->windowed_temporal_buffer = malloc(
				(core->conf->temporal_buffer_size) * sizeof(FLT));
		memset(core->windowed_temporal_buffer, 0,
				core->conf->temporal_buffer_size * sizeof(FLT));
		core->windowed_fft_buffer = malloc(
				(core->conf->fft_size) * sizeof(FLT));
		memset(core->windowed_fft_buffer, 0,
				core->conf->fft_size * sizeof(FLT));

		core->fftplan = lingot_fft_plan_create(core->windowed_fft_buffer,
				core->conf->fft_size);

		/*
		 * 8 order Chebyshev filters, with wc=0.9/i (normalised respect to
		 * Pi). We take 0.9 instead of 1 to leave a 10% of safety margin,
		 * in order to avoid aliased frequencies near to w=Pi, due to non
		 * ideality of the filter.
		 *
		 * The cut frequencies wc=Pi/i, with i=1..20, correspond with the
		 * oversampling factor, avoiding aliasing at decimation.
		 *
		 * Why Chebyshev filters?, for a given order, those filters yield
		 * abrupt falls than other ones as Butterworth, making the most of
		 * the order. Although Chebyshev filters affects more to the phase,
		 * it doesn't matter due to the analysis is made on the signal
		 * power distribution (only module).
		 */
		core->antialiasing_filter = lingot_filter_cheby_design(8, 0.5,
				0.9 / core->conf->oversampling);

		pthread_mutex_init(&core->temporal_buffer_mutex, NULL );

		// ------------------------------------------------------------

		core->running = 1;
	}

	core->freq = 0.0;
	return core;
}

// -----------------------------------------------------------------------

/* Deallocate resources */
void lingot_core_destroy(LingotCore* core) {

	if (core->audio != NULL ) {
		lingot_fft_plan_destroy(core->fftplan);
		lingot_audio_destroy(core->audio);

		free(core->spd_fft);
		free(core->noise_level);
		free(core->SPL);
		free(core->spd_dft);
//		free(core->diff2_spd_fft);
		free(core->flt_read_buffer);
		free(core->temporal_buffer);

		if (core->hamming_window_fft != NULL ) {
			free(core->hamming_window_temporal);
		}

		if (core->windowed_temporal_buffer != NULL ) {
			free(core->windowed_temporal_buffer);
		}

		if (core->hamming_window_fft != NULL ) {
			free(core->hamming_window_fft);
		}

		if (core->windowed_fft_buffer != NULL ) {
			free(core->windowed_fft_buffer);
		}

		if (core->antialiasing_filter != NULL ) {
			lingot_filter_destroy(core->antialiasing_filter);
		}

		pthread_mutex_destroy(&core->temporal_buffer_mutex);
	}

	free(core);
}

// -----------------------------------------------------------------------

// reads a new piece of signal from audio source, applies filtering and
// decimation and appends it to the buffer
int lingot_core_read_callback(FLT* read_buffer, int read_buffer_size, void *arg) {

	unsigned int i, decimation_output_index; // loop variables.
	int decimation_output_len;
	FLT* decimation_in;
	FLT* decimation_out;
	LingotCore* core = (LingotCore*) arg;
	LingotConfig* conf = core->conf;

	memcpy(core->flt_read_buffer, read_buffer, read_buffer_size * sizeof(FLT));

	//	double freq = 100.0;
	//	for (i = 0; i < read_buffer_size; i++) {
	//		core->flt_read_buffer[i] = 5e2 * cos(2.0 * M_PI * freq * i
	//				/ conf->sample_rate);
	//	}

	if (conf->gain_nu != 1.0) {
		for (i = 0; i < read_buffer_size; i++)
			core->flt_read_buffer[i] *= conf->gain_nu;
	}

	//
	// just readed:
	//
	//  ----------------------------
	// |bxxxbxxxbxxxbxxxbxxxbxxxbxxx|
	//  ----------------------------
	//
	// <----------------------------> read_buffer_size*oversampling
	//

	decimation_output_len = 1
			+ (read_buffer_size - (decimation_input_index + 1))
					/ conf->oversampling;

	pthread_mutex_lock(&core->temporal_buffer_mutex);

	/* we shift the temporal window to leave a hollow where place the new piece
	 of data read. The buffer is actually a queue. */
	if (conf->temporal_buffer_size > decimation_output_len) {
		memmove(core->temporal_buffer,
				&core->temporal_buffer[decimation_output_len],
				(conf->temporal_buffer_size - decimation_output_len)
						* sizeof(FLT));
	}

	//
	// previous buffer situation:
	//
	//  ------------------------------------------
	// | xxxxxxxxxxxxxxxxxxxxxx | yyyyy | aaaaaaa |
	//  ------------------------------------------
	//                                    <------> read_buffer_size
	//                           <---------------> fft_size
	//  <----------------------------------------> temporal_buffer_size
	//
	// new situation:
	//
	//  ------------------------------------------
	// | xxxxxxxxxxxxxxxxyyyyaa | aaaaa |         |
	//  ------------------------------------------
	//

	// decimation with lowpass filtering

	/* we decimate the read signal and put it at the end of the buffer. */
	if (conf->oversampling > 1) {

		decimation_in = core->flt_read_buffer;
		decimation_out = &core->temporal_buffer[conf->temporal_buffer_size
				- decimation_output_len];

		// low pass filter to avoid aliasing.
		lingot_filter_filter(core->antialiasing_filter, read_buffer_size,
				decimation_in, decimation_in);

		// compression.
		for (decimation_output_index = 0;
				decimation_input_index < read_buffer_size;
				decimation_output_index++, decimation_input_index +=
						conf->oversampling)
			decimation_out[decimation_output_index] =
					decimation_in[decimation_input_index];
		decimation_input_index -= read_buffer_size;
	} else
		memcpy(
				&core->temporal_buffer[conf->temporal_buffer_size
						- decimation_output_len], core->flt_read_buffer,
				decimation_output_len * sizeof(FLT));
	//
	//  ------------------------------------------
	// | xxxxxxxxxxxxxxxxyyyyaa | aaaaa | bbbbbbb |
	//  ------------------------------------------
	//

	pthread_mutex_unlock(&core->temporal_buffer_mutex);

	return 0;
}

void lingot_core_compute_fundamental_fequency(LingotCore* core) {

	register unsigned int i, k; // loop variables.
	const LingotConfig* conf = core->conf;
	const FLT delta_f_FFT = ((FLT) conf->sample_rate)
			/ (conf->oversampling * conf->fft_size); // FFT resolution in Hz.

	// ----------------- TRANSFORMATION TO FREQUENCY DOMAIN ----------------

	pthread_mutex_lock(&core->temporal_buffer_mutex);

	// windowing
	if (conf->window_type != NONE) {
		for (i = 0; i < conf->fft_size; i++) {
			core->windowed_fft_buffer[i] =
					core->temporal_buffer[conf->temporal_buffer_size
							- conf->fft_size + i] * core->hamming_window_fft[i];
		}
	} else {
		memmove(core->windowed_fft_buffer,
				&core->temporal_buffer[conf->temporal_buffer_size
						- conf->fft_size], conf->fft_size * sizeof(FLT));
	}

	int spd_size =
			(conf->fft_size > 256) ? (conf->fft_size >> 1) : conf->fft_size;

	lingot_fft_compute_dft_and_spd(core->fftplan, core->spd_fft, spd_size);

	// representable piece
	for (i = 0; i < spd_size; i++) {
		core->SPL[i] = 10.0 * log10(core->spd_fft[i]);
	}
	// TODO: filter width
	lingot_signal_compute_noise_level(core->SPL, spd_size, 30,
			core->noise_level);

	unsigned int lowest_index = (unsigned int) ceil(
			conf->min_frequency * (1.0 * conf->oversampling / conf->sample_rate)
					* conf->fft_size);

	// TODO: min SNR
	short divisor = 1;
	FLT f0 = lingot_signal_get_fundamental_peak2(core->SPL,
			core->fftplan->fft_out, core->noise_level, conf->fft_size >> 1, 6,
			lowest_index, conf->peak_half_width, delta_f_FFT, 15.0, 20.0,
			conf->min_frequency, core, &divisor);

	if (f0 == 0.0) {
		core->freq = 0.0;
		pthread_mutex_unlock(&core->temporal_buffer_mutex);
		return;
	}

	FLT w = 2 * M_PI * f0 * conf->oversampling / conf->sample_rate;

	// windowing
	if (conf->window_type != NONE) {
		for (i = 0; i < conf->temporal_buffer_size; i++) {
			core->windowed_temporal_buffer[i] = core->temporal_buffer[i]
					* core->hamming_window_temporal[i];
		}
	} else {
		memmove(core->windowed_temporal_buffer, core->temporal_buffer,
				conf->temporal_buffer_size * sizeof(FLT));
	}

	pthread_mutex_unlock(&core->temporal_buffer_mutex);

	//  Maximum finding by Newton-Raphson
	// -----------------------------------

	FLT wk = -1.0e5;
	FLT wkm1 = w;
	// first iterator set to the current approximation.
	FLT d1_SPD, d2_SPD;

	for (k = 0; (k < conf->max_nr_iter) && (fabs(wk - wkm1) > 1.0e-8); k++) {
		wk = wkm1;

		// ! we use the WHOLE temporal window for bigger precission.
		lingot_fft_spd_diffs_eval(core->windowed_temporal_buffer,
				conf->temporal_buffer_size, wk, &d1_SPD, &d2_SPD);
		wkm1 = wk - d1_SPD / d2_SPD;
//		printf("%f %f %f\n", wk, d1_SPD, d2_SPD);
	}
//	printf("%d\n", k);

	w = wkm1; // frequency in rads.
	core->freq = (w * conf->sample_rate)
			/ (divisor * 2.0 * M_PI * conf->oversampling); // analog frequency.
}

/* start running the core in another thread */
void lingot_core_start(LingotCore* core) {

	int audio_status = 0;
	decimation_input_index = 0;

	if (core->audio != NULL ) {
		audio_status = lingot_audio_start(core->audio);

		if (audio_status == 0) {
			pthread_mutex_init(&core->thread_computation_mutex, NULL );
			pthread_cond_init(&core->thread_computation_cond, NULL );

			pthread_attr_init(&core->thread_computation_attr);
			pthread_create(&core->thread_computation,
					&core->thread_computation_attr,
					(void* (*)(void*)) lingot_core_run_computation_thread,
					core);
			core->running = 1;
		} else {
			core->running = 0;
			lingot_audio_destroy(core->audio);
			core->audio = 0x0;
		}

	}
}

/* stop running the core */
void lingot_core_stop(LingotCore* core) {
	void* thread_result;

	int result;
	struct timeval tout, tout_abs;
	struct timespec tout_tspec;

	gettimeofday(&tout_abs, NULL );
	tout.tv_sec = 0;
	tout.tv_usec = 300000;

	if (core->running == 1) {
		core->running = 0;

		timeradd(&tout, &tout_abs, &tout_abs);
		tout_tspec.tv_sec = tout_abs.tv_sec;
		tout_tspec.tv_nsec = 1000 * tout_abs.tv_usec;

		// watchdog timer
		pthread_mutex_lock(&core->thread_computation_mutex);
		result = pthread_cond_timedwait(&core->thread_computation_cond,
				&core->thread_computation_mutex, &tout_tspec);
		pthread_mutex_unlock(&core->thread_computation_mutex);

		if (result == ETIMEDOUT) {
			fprintf(stderr, "warning: cancelling computation thread\n");
			pthread_cancel(core->thread_computation);
		} else {
			pthread_join(core->thread_computation, &thread_result);
		}
		pthread_attr_destroy(&core->thread_computation_attr);
		pthread_mutex_destroy(&core->thread_computation_mutex);
		pthread_cond_destroy(&core->thread_computation_cond);

		memset(core->SPL, 0,
				((core->conf->fft_size > 256) ?
						(core->conf->fft_size >> 1) : core->conf->fft_size)
						* sizeof(FLT));
		core->freq = 0.0;
	}

	if (core->audio != NULL ) {
		lingot_audio_stop(core->audio);
	}
}

/* run the core */
void lingot_core_run_computation_thread(LingotCore* core) {
	struct timeval tout, tout_abs;
	struct timespec tout_tspec;

	gettimeofday(&tout_abs, NULL );
	tout.tv_sec = 0;
	tout.tv_usec = 1e6 / core->conf->calculation_rate;

	while (core->running) {
		lingot_core_compute_fundamental_fequency(core);
		timeradd(&tout, &tout_abs, &tout_abs);
		tout_tspec.tv_sec = tout_abs.tv_sec;
		tout_tspec.tv_nsec = 1000 * tout_abs.tv_usec;
		pthread_mutex_lock(&core->thread_computation_mutex);
		pthread_cond_timedwait(&core->thread_computation_cond,
				&core->thread_computation_mutex, &tout_tspec);
		pthread_mutex_unlock(&core->thread_computation_mutex);

		if (core->audio != NULL ) {
			if (core->audio->interrupted) {
				memset(core->SPL, 0,
						((core->conf->fft_size > 256) ?
								(core->conf->fft_size >> 1) :
								core->conf->fft_size) * sizeof(FLT));
				core->freq = 0.0;
				core->running = 0;
			}
		}
	}

	pthread_mutex_lock(&core->thread_computation_mutex);
	pthread_cond_broadcast(&core->thread_computation_cond);
	pthread_mutex_unlock(&core->thread_computation_mutex);
}
