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

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>

#include "lingot-defs-internal.h"
#include "lingot-filter.h"
#include "lingot-audio.h"
#include "lingot-fft.h"
#include "lingot-signal.h"
#include "lingot-core.h"
#include "lingot-config.h"
#include "lingot-i18n.h"
#include "lingot-msg.h"

// ----------------------------------------------------------------------------

typedef struct {

    //  -- results --
    LINGOT_FLT freq; // computed analog frequency.
    LINGOT_FLT* SPL; // visual portion of FFT.
    //  -- shared data --

    LINGOT_FLT* SPL_thread_safe; // copy for thread-safe access.

    lingot_audio_handler_t audio; // audio handler.

    LINGOT_FLT* flt_read_buffer;
    LINGOT_FLT* temporal_buffer; // sample memory.

    // precomputed hamming windows
    LINGOT_FLT* hamming_window_temporal;
    LINGOT_FLT* hamming_window_fft;

    // windowed signals
    LINGOT_FLT* windowed_temporal_buffer;
    LINGOT_FLT* windowed_fft_buffer;

    // spectral power distribution estimation.
    LINGOT_FLT* noise_level;

    lingot_fft_plan_t fftplan;

    lingot_filter_t antialiasing_filter; // antialiasing filter for decimation.

    int running;

    unsigned int decimation_input_index;

    lingot_config_t conf; // configuration structure

    pthread_t thread_computation;
    pthread_attr_t thread_computation_attr;
    pthread_cond_t thread_computation_cond;
    pthread_mutex_t thread_computation_mutex;

    // Synchronized access to the results (typically accessed by computation and UI threads)
    pthread_mutex_t results_mutex;

    // Synchronized access to the audio buffer (accessed by computation and audio threads)
    pthread_mutex_t temporal_buffer_mutex;
} lingot_core_private_t;

// ----------------------------------------------------------------------------

void lingot_core_read_callback(LINGOT_FLT* read_buffer, unsigned int samples_read, void *arg);

void lingot_core_new(lingot_core_t* core_, lingot_config_t* conf) {

    char buff[1000];

    core_->core_private = malloc(sizeof(lingot_core_private_t));
    lingot_core_private_t* core = (lingot_core_private_t*) core_->core_private;

    lingot_config_copy(&core->conf, conf);
    core->running = 0;
    core->noise_level = NULL;
    core->SPL = NULL;
    core->SPL_thread_safe = NULL;
    core->flt_read_buffer = NULL;
    core->temporal_buffer = NULL;
    core->windowed_temporal_buffer = NULL;
    core->windowed_fft_buffer = NULL;
    core->hamming_window_temporal = NULL;
    core->hamming_window_fft = NULL;

    unsigned int requested_sample_rate = (unsigned int) core->conf.sample_rate;

    if (core->conf.sample_rate <= 0) {
        core->conf.sample_rate = 0;
    }

    lingot_audio_new(&core->audio,
                     core->conf.audio_system_index,
                     core->conf.audio_dev[core->conf.audio_system_index],
            core->conf.sample_rate, lingot_core_read_callback, core);

    if (core->audio.audio_system != -1) {

        if (requested_sample_rate != core->audio.real_sample_rate) {
            core->conf.sample_rate = (int) core->audio.real_sample_rate;
            lingot_config_update_internal_params(conf);
            //			sprintf(buff,
            //					_("The requested sample rate is not available, the real sample rate has been set to %d Hz"),
            //					core->audio.real_sample_rate);
            //			lingot_msg_add_warning(buff);
        }

        if (core->conf.temporal_buffer_size < core->conf.fft_size) {
            core->conf.temporal_window = ((double) core->conf.fft_size
                                          * core->conf.oversampling) / core->conf.sample_rate;
            core->conf.temporal_buffer_size = core->conf.fft_size;
            lingot_config_update_internal_params(conf);
            snprintf(buff, sizeof(buff),
                     _("The temporal buffer is smaller than FFT size. It has been increased to %0.3f seconds"),
                     core->conf.temporal_window);
            lingot_msg_add_warning(buff);
        }

        // Since the SPD is symmetrical, we only store the 1st half.
        const unsigned int spd_size = (core->conf.fft_size / 2);

        core->noise_level = malloc(spd_size * sizeof(LINGOT_FLT));
        core->SPL = malloc(spd_size * sizeof(LINGOT_FLT));
        core->SPL_thread_safe = malloc(spd_size * sizeof(LINGOT_FLT));

        memset(core->noise_level, 0, spd_size * sizeof(LINGOT_FLT));
        memset(core->SPL, 0, spd_size * sizeof(LINGOT_FLT));
        memset(core->SPL_thread_safe, 0, spd_size * sizeof(LINGOT_FLT));

        // audio source read in floating point format.
        core->flt_read_buffer = malloc(core->audio.read_buffer_size_samples * sizeof(LINGOT_FLT));
        memset(core->flt_read_buffer, 0, core->audio.read_buffer_size_samples * sizeof(LINGOT_FLT));

        // stored samples.
        core->temporal_buffer = malloc((core->conf.temporal_buffer_size) * sizeof(LINGOT_FLT));
        memset(core->temporal_buffer, 0, core->conf.temporal_buffer_size * sizeof(LINGOT_FLT));

        core->hamming_window_temporal = NULL;
        core->hamming_window_fft = NULL;

        if (core->conf.window_type != NONE) {
            core->hamming_window_temporal = malloc((core->conf.temporal_buffer_size) * sizeof(LINGOT_FLT));
            core->hamming_window_fft = malloc((core->conf.fft_size) * sizeof(LINGOT_FLT));

            lingot_signal_window((int) core->conf.temporal_buffer_size,
                                 core->hamming_window_temporal,
                                 core->conf.window_type);
            lingot_signal_window((int) core->conf.fft_size,
                                 core->hamming_window_fft,
                                 core->conf.window_type);
        }

        core->decimation_input_index = 0;

        core->windowed_temporal_buffer = malloc((core->conf.temporal_buffer_size) * sizeof(LINGOT_FLT));
        memset(core->windowed_temporal_buffer, 0, core->conf.temporal_buffer_size * sizeof(LINGOT_FLT));
        core->windowed_fft_buffer = malloc((core->conf.fft_size) * sizeof(LINGOT_FLT));
        memset(core->windowed_fft_buffer, 0, core->conf.fft_size * sizeof(LINGOT_FLT));

        lingot_fft_plan_create(&core->fftplan, core->windowed_fft_buffer,
                               core->conf.fft_size);

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
         * the order. Although Chebyshev filters affect more to the phase,
         * it doesn't matter due to the analysis is made on the signal
         * power distribution (only magnitude).
         */
        lingot_filter_cheby_design(&core->antialiasing_filter, 8, 0.5,
                                   0.9 / core->conf.oversampling);

        pthread_mutex_init(&core->temporal_buffer_mutex, NULL);
        pthread_mutex_init(&core->results_mutex, NULL);

        // ------------------------------------------------------------

        //        core->running = 1;
    }

    core->freq = 0.0;
}

// -----------------------------------------------------------------------

/* Deallocate resources */
void lingot_core_destroy(lingot_core_t* core_) {

    lingot_core_private_t* core = (lingot_core_private_t*) core_->core_private;
    if (core && core->audio.audio_system != -1) {
        lingot_fft_plan_destroy(&core->fftplan);
        lingot_audio_destroy(&core->audio);

        free(core->noise_level);
        free(core->SPL);
        free(core->SPL_thread_safe);
        free(core->flt_read_buffer);
        free(core->temporal_buffer);

        free(core->hamming_window_temporal);
        free(core->windowed_temporal_buffer);
        free(core->hamming_window_fft);
        free(core->windowed_fft_buffer);

        core->noise_level = NULL;
        core->SPL = NULL;
        core->SPL_thread_safe = NULL;
        core->flt_read_buffer = NULL;
        core->temporal_buffer = NULL;
        core->hamming_window_temporal = NULL;
        core->windowed_temporal_buffer = NULL;
        core->hamming_window_fft = NULL;
        core->windowed_fft_buffer = NULL;

        lingot_filter_destroy(&core->antialiasing_filter);

        pthread_mutex_destroy(&core->temporal_buffer_mutex);
        pthread_mutex_destroy(&core->results_mutex);
    }
    free(core_->core_private);
    core_->core_private = NULL;
}

// -----------------------------------------------------------------------

// Method being called by the audio thread with a new audio piece. We apply filtering and
// decimation and append it to the temporal buffer.
void lingot_core_read_callback(LINGOT_FLT* read_buffer, unsigned int samples_read, void *arg) {

    unsigned int decimation_output_index; // loop variables.
    unsigned int decimation_output_len;
    LINGOT_FLT* decimation_in;
    LINGOT_FLT* decimation_out;
    lingot_core_private_t* core = (lingot_core_private_t*) arg;
    const lingot_config_t* const conf = &core->conf;

    memcpy(core->flt_read_buffer, read_buffer, samples_read * sizeof(LINGOT_FLT));
    //	double omega = 2.0 * M_PI * 100.0;
    //	double T = 1.0 / conf->sample_rate;
    //	static double t = 0.0;
    //	for (i = 0; i < read_buffer_size; i++) {
    //		core->flt_read_buffer[i] = 1e4
    //				* (cos(omega * t) + (0.5 * rand()) / RAND_MAX);
    //		t += T;
    //	}

    //	if (conf->gain_nu != 1.0) {
    //		for (i = 0; i < read_buffer_size; i++)
    //			core->flt_read_buffer[i] *= conf->gain_nu;
    //	}

    //
    // just read:
    //
    //  ----------------------------
    // |bxxxbxxxbxxxbxxxbxxxbxxxbxxx|
    //  ----------------------------
    //
    // <----------------------------> samples_read
    //

    decimation_output_len = 1
            + (samples_read - (core->decimation_input_index + 1))
            / conf->oversampling;

    // we synchronize the access to the temporal buffer
    pthread_mutex_lock(&core->temporal_buffer_mutex);

    /* we shift the temporal window to leave a hollow where place the new piece
     of data read. The buffer is actually a queue. */
    if (conf->temporal_buffer_size > decimation_output_len) {
        memmove(core->temporal_buffer,
                &core->temporal_buffer[decimation_output_len],
                (conf->temporal_buffer_size - decimation_output_len)
                * sizeof(LINGOT_FLT));
    }

    //
    // previous buffer situation:
    //
    //  ------------------------------------------
    // | xxxxxxxxxxxxxxxxxxxxxx | yyyyy | aaaaaaa |
    //  ------------------------------------------
    //                                    <------> samples_read/oversampling
    //                           <---------------> fft_size
    //  <----------------------------------------> temporal_buffer_size
    //
    // new situation:
    //
    //  ------------------------------------------
    // | xxxxxxxxxxxxxxxxyyyyaa | aaaaa |         |
    //  ------------------------------------------
    //

    // decimation with low-pass filtering

    /* we decimate the signal and append it at the end of the buffer. */
    if (conf->oversampling > 1) {

        decimation_in = core->flt_read_buffer;
        decimation_out = &core->temporal_buffer[conf->temporal_buffer_size
                - decimation_output_len];

        // low pass filter to avoid aliasing.
        lingot_filter_filter(&core->antialiasing_filter, samples_read,
                             decimation_in, decimation_in);

        // downsampling.
        for (decimation_output_index = 0; core->decimation_input_index < samples_read;
             decimation_output_index++, core->decimation_input_index +=
             conf->oversampling) {
            decimation_out[decimation_output_index] =
                    decimation_in[core->decimation_input_index];
        }
        core->decimation_input_index -= samples_read;
    } else {
        memcpy(
                    &core->temporal_buffer[conf->temporal_buffer_size
                - decimation_output_len], core->flt_read_buffer,
                decimation_output_len * sizeof(LINGOT_FLT));
    }
    //
    //  ------------------------------------------
    // | xxxxxxxxxxxxxxxxyyyyaa | aaaaa | bbbbbbb |
    //  ------------------------------------------
    //

    pthread_mutex_unlock(&core->temporal_buffer_mutex);

#ifdef DUMP
    static FILE* fid1 = 0x0;
    static FILE* fid2 = 0x0;

    if (fid1 == 0x0) {
        fid1 = fopen("/tmp/dump_post_filter.txt", "w");
        fid2 = fopen("/tmp/dump_post.txt", "w");
    }

    for (i = 0; i < samples_read; i++) {
        fprintf(fid1, "%f ", core->flt_read_buffer[i]);
    }
    decimation_out = &core->temporal_buffer[conf->temporal_buffer_size
            - decimation_output_len];
    for (i = 0; i < decimation_output_len; i++) {
        fprintf(fid2, "%f ", decimation_out[i]);
    }
#endif
}

void lingot_core_compute_fundamental_fequency(lingot_core_t *core_) {

    unsigned int i, k; // loop variables.

    lingot_core_private_t* core = (lingot_core_private_t*) ((lingot_core_t*) core_)->core_private;

    const lingot_config_t* const conf = &core->conf;
    const LINGOT_FLT index2f = ((LINGOT_FLT) conf->sample_rate)
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
                - conf->fft_size], conf->fft_size * sizeof(LINGOT_FLT));
    }

    const unsigned int spd_size = (conf->fft_size / 2);

    // we synchronize the access to the SPL buffer
    pthread_mutex_lock(&core->results_mutex);

    // FFT
    lingot_fft_compute_dft_and_spd(&core->fftplan, core->SPL, spd_size);

    static const LINGOT_FLT minSPL = -200;
    for (i = 0; i < spd_size; i++) {
        core->SPL[i] = 10.0 * log10(core->SPL[i]);
        if (core->SPL[i] < minSPL) {
            core->SPL[i] = minSPL;
        }
    }

    LINGOT_FLT noise_filter_width = 150.0; // hz
    unsigned int noise_filter_width_samples = (unsigned int) ceil(
                noise_filter_width * conf->fft_size * conf->oversampling
                / conf->sample_rate);

    lingot_signal_compute_noise_level(core->SPL,
                                      (int) spd_size,
                                      (int) noise_filter_width_samples,
                                      core->noise_level);
    for (i = 0; i < spd_size; i++) {
        core->SPL[i] -= core->noise_level[i];
    }

    pthread_mutex_unlock(&core->results_mutex);

    unsigned int lowest_index = (unsigned int) ceil(
                conf->internal_min_frequency
                * (1.0 * conf->oversampling / conf->sample_rate)
                * conf->fft_size);
    unsigned int highest_index = (unsigned int) ceil(0.95 * spd_size);

    short divisor = 1;
    LINGOT_FLT f0 = lingot_signal_estimate_fundamental_frequency(core->SPL,
                                                                 0.5 * core->freq,
                                                                 core->fftplan.fft_out,
                                                                 spd_size,
                                                                 conf->peak_number,
                                                                 lowest_index,
                                                                 highest_index,
                                                                 (unsigned short) conf->peak_half_width,
                                                                 index2f,
                                                                 conf->min_SNR,
                                                                 conf->min_overall_SNR,
                                                                 conf->internal_min_frequency,
                                                                 &divisor);

    LINGOT_FLT w;
    LINGOT_FLT w0 = (f0 == 0.0) ?
                0.0 :
                2 * M_PI * f0 * conf->oversampling / conf->sample_rate;
    w = w0;
    //	int Mi = floor(w / index2w);

    if (w != 0.0) {
        // windowing
        if (conf->window_type != NONE) {
            for (i = 0; i < conf->temporal_buffer_size; i++) {
                core->windowed_temporal_buffer[i] = core->temporal_buffer[i]
                        * core->hamming_window_temporal[i];
            }
        } else {
            memmove(core->windowed_temporal_buffer, core->temporal_buffer,
                    conf->temporal_buffer_size * sizeof(LINGOT_FLT));
        }
    }

    pthread_mutex_unlock(&core->temporal_buffer_mutex); // we don't need the read buffer anymore

    if (w != 0.0) {

        //  Maximum finding by Newton-Raphson
        // -----------------------------------

        LINGOT_FLT wk = -1.0e5;
        LINGOT_FLT wkm1 = w;
        // first iterator set to the current approximation.
        LINGOT_FLT d0_SPD = 0.0;
        LINGOT_FLT d1_SPD = 0.0;
        LINGOT_FLT d2_SPD = 0.0;
        LINGOT_FLT d0_SPD_old = 0.0;

        //		printf("NR iter: %f ", w * w2f);

        for (k = 0; (k < conf->max_nr_iter) && (fabs(wk - wkm1) > 1.0e-4);
             k++) {
            wk = wkm1;

            d0_SPD_old = d0_SPD;
            lingot_fft_spd_diffs_eval(core->windowed_fft_buffer, // TODO: iterate over this buffer?
                                      conf->fft_size, wk, &d0_SPD, &d1_SPD, &d2_SPD);

            wkm1 = wk - d1_SPD / d2_SPD;
            //			printf(" -> (%f,%g,%g,%g)", wkm1 * w2f, d0_SPD, d1_SPD, d2_SPD);

            if (d0_SPD < d0_SPD_old) {
                //				printf("!!!", d0_SPD, d0_SPD_old);
                wkm1 = 0.0;
                break;
            }

        }
        //		printf("\n");

        if (wkm1 > 0.0) {
            w = wkm1; // frequency in rads.
            wk = -1.0e5;
            d0_SPD = 0.0;
            //			printf("NR2 iter: %f ", w * w2f);

            for (k = 0;
                 (k <= 1)
                 || ((k < conf->max_nr_iter)
                     && (fabs(wk - wkm1) > 1.0e-4)); k++) {
                wk = wkm1;

                // ! we use the WHOLE temporal window for bigger precision.
                d0_SPD_old = d0_SPD;
                lingot_fft_spd_diffs_eval(core->windowed_temporal_buffer,
                                          conf->temporal_buffer_size, wk, &d0_SPD, &d1_SPD,
                                          &d2_SPD);

                wkm1 = wk - d1_SPD / d2_SPD;
                //				printf(" -> (%f,%g,%g,%g)", wkm1 * w2f, d0_SPD, d1_SPD, d2_SPD);

                if (d0_SPD < d0_SPD_old) {
                    //					printf("!!!");
                    wkm1 = 0.0;
                    break;
                }

            }
            //			printf("\n");

            if (wkm1 > 0.0) {
                w = wkm1; // frequency in rads.
            }
        }
    }

    // analog frequency in Hz.
    LINGOT_FLT freq = (w == 0.0) ?
                0.0 :
                w * conf->sample_rate
                / (divisor * 2.0 * M_PI * conf->oversampling);
    LINGOT_FLT freq_aux = lingot_signal_frequency_locker(freq, core->conf.internal_min_frequency);

    pthread_mutex_lock(&core->results_mutex);
    core->freq = freq_aux;
    pthread_mutex_unlock(&core->results_mutex);
}

static void* lingot_core_mainloop_(void* core_) {
    lingot_core_mainloop((lingot_core_t*) core_);
    return NULL;
}

void lingot_core_mainloop(lingot_core_t* core_) {
    struct timeval tout_abs;
    struct timespec tout_tspec;

    lingot_core_private_t* core = (lingot_core_private_t*) ((lingot_core_t*) core_)->core_private;
    gettimeofday(&tout_abs, NULL);

    while (core->running) {
        lingot_core_compute_fundamental_fequency(core_);
        tout_abs.tv_usec += (long) (1e6 / core->conf.calculation_rate);
        if (tout_abs.tv_usec >= 1000000) {
            tout_abs.tv_usec -= 1000000;
            tout_abs.tv_sec++;
        }
        tout_tspec.tv_sec = tout_abs.tv_sec;
        tout_tspec.tv_nsec = 1000 * tout_abs.tv_usec;
        pthread_mutex_lock(&core->thread_computation_mutex);
        pthread_cond_timedwait(&core->thread_computation_cond,
                               &core->thread_computation_mutex, &tout_tspec);
        pthread_mutex_unlock(&core->thread_computation_mutex);

        if (core->audio.audio_system != -1) {
            const unsigned int spd_size = core->conf.fft_size / 2;
            if (core->audio.interrupted) {
                memset(core->SPL, 0, spd_size * sizeof(LINGOT_FLT));
                core->freq = 0.0;
                core->running = 0;
            }
        }
    }

    pthread_mutex_lock(&core->thread_computation_mutex);
    pthread_cond_broadcast(&core->thread_computation_cond);
    pthread_mutex_unlock(&core->thread_computation_mutex);
}


void lingot_core_start(lingot_core_t* core_)
{
    int audio_status = 0;
    lingot_core_private_t* core = (lingot_core_private_t*) core_->core_private;
    if (core && !core->running && (core->audio.audio_system != -1)) {
        core->decimation_input_index = 0;
        audio_status = lingot_audio_start(&core->audio);
        if (audio_status == 0) {
            core->running = 1;
        } else {
            core->running = 0;
            lingot_audio_destroy(&core->audio);
        }
    }
}

void lingot_core_stop(lingot_core_t* core_)
{
    lingot_core_private_t* core = (lingot_core_private_t*) core_->core_private;
    if (core && core->running) {
        core->running = 0;
        if (core->audio.audio_system != -1) {
            lingot_audio_stop(&core->audio);
        }
    }
}

void lingot_core_thread_start(lingot_core_t* core_) {

    lingot_core_private_t* core = (lingot_core_private_t*) core_->core_private;
    if (core && !core->running) {
        lingot_core_start(core_);
        if (core->running) {
            pthread_mutex_init(&core->thread_computation_mutex, NULL);
            pthread_cond_init(&core->thread_computation_cond, NULL);

            pthread_attr_init(&core->thread_computation_attr);
            pthread_create(&core->thread_computation,
                           &core->thread_computation_attr,
                           lingot_core_mainloop_,
                           core_);
        }
    }
}

void lingot_core_thread_stop(lingot_core_t* core_) {
    void* thread_result;

    int result;
    struct timeval tout_abs;
    struct timespec tout_tspec;

    gettimeofday(&tout_abs, NULL);

    lingot_core_private_t* core = (lingot_core_private_t*) core_->core_private;
    if (core && core->running) {
        lingot_core_stop(core_);

        if (!core->running) {

            tout_abs.tv_usec += 300000;
            if (tout_abs.tv_usec >= 1000000) {
                tout_abs.tv_usec -= 1000000;
                tout_abs.tv_sec++;
            }

            tout_tspec.tv_sec = tout_abs.tv_sec;
            tout_tspec.tv_nsec = 1000 * tout_abs.tv_usec;

            // watchdog timer
            pthread_mutex_lock(&core->thread_computation_mutex);
            result = pthread_cond_timedwait(&core->thread_computation_cond,
                                            &core->thread_computation_mutex, &tout_tspec);
            pthread_mutex_unlock(&core->thread_computation_mutex);

            if (result == ETIMEDOUT) {
                fprintf(stderr, "warning: cancelling computation thread\n");
                //			pthread_cancel(core->thread_computation);
            } else {
                pthread_join(core->thread_computation, &thread_result);
            }
            pthread_attr_destroy(&core->thread_computation_attr);
            pthread_mutex_destroy(&core->thread_computation_mutex);
            pthread_cond_destroy(&core->thread_computation_cond);

            core->freq = 0.0;
            if (core->SPL) {
                unsigned int spd_size = core->conf.fft_size / 2;
                memset(core->SPL, 0, spd_size * sizeof(LINGOT_FLT));
            }
        }
    }
}

int lingot_core_thread_is_running(const lingot_core_t *core_)
{
    const lingot_core_private_t* core = (const lingot_core_private_t*) ((lingot_core_t*) core_)->core_private;
    return core->running;
}

LINGOT_FLT lingot_core_thread_get_result_frequency(lingot_core_t* core_) {

    lingot_core_private_t* core = (lingot_core_private_t*) ((lingot_core_t*) core_)->core_private;
    // TODO: mutex not required
    pthread_mutex_lock(&core->results_mutex);
    LINGOT_FLT freq = core->freq;
    pthread_mutex_unlock(&core->results_mutex);

    return freq;
}

LINGOT_FLT* lingot_core_thread_get_result_spd(lingot_core_t* core_) {

    lingot_core_private_t* core = (lingot_core_private_t*) ((lingot_core_t*) core_)->core_private;
    pthread_mutex_lock(&core->results_mutex);
    memcpy(core->SPL_thread_safe, core->SPL, (core->conf.fft_size/2) * sizeof(LINGOT_FLT));
    pthread_mutex_unlock(&core->results_mutex);

    return core->SPL_thread_safe;
}
