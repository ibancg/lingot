//-*- C++ -*-
/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2007  Ibán Cereijo Graña, Jairo Chapela Martínez.
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

#ifndef LIB_FFTW
# include "lingot-complex.h"
#endif

#include "lingot-fft.h"
#include "lingot-signal.h"
#include "lingot-core.h"
#include "lingot-config.h"

LingotCore* lingot_core_new(LingotConfig* conf) {

	LingotCore* core = malloc(sizeof(LingotCore));

	core->conf = conf;
	core->running = 1;

# ifdef LIBSNDOBJ
#  ifdef OSS
	core->audio = new SndRTIO(1, SND_INPUT, 512, 512, SHORTSAM_LE, NULL,
			core->conf->read_buffer_size*core->conf->oversampling, core->conf->sample_rate, core->conf->audio_dev);
#  endif		  
#  ifdef ALSA
	core->audio = new SndRTIO(1, SND_INPUT, 512, 4, SHORTSAM_LE, NULL,
			core->conf->read_buffer_size*core->conf->oversampling, core->conf->sample_rate, "plughw:0,0");
#  endif		  
# else
	// creates an audio handler.
	core->audio = lingot_audio_new(1, core->conf->sample_rate, SAMPLE_FORMAT,
			core->conf->audio_dev);
# endif

	// since the SPD is simmetrical, we only store the 1st half.
	if (core->conf->fft_size > 256) {
		core->spd_fft = malloc((core->conf->fft_size >> 1)*sizeof(FLT));
		core->X = malloc((core->conf->fft_size >> 1)*sizeof(FLT));
		memset(core->spd_fft, 0, (core->conf->fft_size >> 1)*sizeof(FLT));
		memset(core->X, 0, (core->conf->fft_size >> 1)*sizeof(FLT));
	} else { // if the fft size is 256, we store the whole signal for representation.
		core->spd_fft = malloc((core->conf->fft_size)*sizeof(FLT));
		core->X = malloc((core->conf->fft_size)*sizeof(FLT));
		memset(core->spd_fft, 0, core->conf->fft_size*sizeof(FLT));
		memset(core->X, 0, (core->conf->fft_size)*sizeof(FLT));
	}

	core->spd_dft = malloc((core->conf->dft_size)*sizeof(FLT));
	memset(core->spd_dft, 0, core->conf->dft_size*sizeof(FLT));

	core->diff2_spd_fft = malloc(core->conf->fft_size*sizeof(FLT)); // 2nd derivative from SPD.
	memset(core->diff2_spd_fft, 0, core->conf->fft_size*sizeof(FLT));

	memset(core->spd_dft, 0, core->conf->dft_size*sizeof(FLT));

#ifndef LIB_FFTW  
	lingot_fft_create_phase_factors(conf); // creates the phase factors for FFT.
	core->fft_out = malloc((core->conf->fft_size)*sizeof(LingotComplex)); // complex signal in freq domain.
	memset(core->fft_out, 0, core->conf->fft_size*sizeof(LingotComplex));
#else
	core->fftw_in = new fftw_complex[core->conf->fft_size];
	memset(core->fftw_in, 0, core->conf->fft_size*sizeof(fftw_complex));
	core->fftw_out = new fftw_complex[core->conf->fft_size];
	memset(core->fftw_out, 0, core->conf->fft_size*sizeof(fftw_complex));
	core->fftwplan = fftw_create_plan(core->conf->fft_size, FFTW_FORWARD, FFTW_ESTIMATE);
#endif

	// read buffer from soundcard.
	core->read_buffer= malloc((core->conf->read_buffer_size
			*core->conf->oversampling)*sizeof(SAMPLE_TYPE));
	memset(core->read_buffer, 0, (core->conf->read_buffer_size
			*core->conf->oversampling)*sizeof(SAMPLE_TYPE));

	// read buffer from soundcard in floating point format.
	core->flt_read_buffer= malloc((core->conf->read_buffer_size
			*core->conf->oversampling)*sizeof(FLT));
	memset(core->flt_read_buffer, 0, (core->conf->read_buffer_size
			*core->conf->oversampling)*sizeof(FLT));

	// stored samples.
	core->temporal_buffer = malloc((core->conf->temporal_buffer_size)
			*sizeof(FLT));
	memset(core->temporal_buffer, 0, core->conf->temporal_buffer_size
			*sizeof(FLT));

	core->hamming_window_temporal = malloc((core->conf->temporal_buffer_size)
			*sizeof(FLT));
	lingot_signal_hamming_window(core->conf->temporal_buffer_size,
			core->hamming_window_temporal);
	core->hamming_window_fft = malloc((core->conf->fft_size) *sizeof(FLT));
	lingot_signal_hamming_window(core->conf->fft_size, core->hamming_window_fft);

	core->windowed_temporal_buffer = malloc((core->conf->temporal_buffer_size)
			*sizeof(FLT));
	memset(core->windowed_temporal_buffer, 0, core->conf->temporal_buffer_size
			*sizeof(FLT));
	core->windowed_fft_buffer = malloc((core->conf->fft_size) *sizeof(FLT));
	memset(core->windowed_fft_buffer, 0, core->conf->fft_size *sizeof(FLT));

	/*
	 * 8 order Chebyshev filters, with wc=0.9/i (normalized respect to
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
	core->antialiasing_filter= lingot_filter_cheby_design(8, 0.5, 0.9
			/core->conf->oversampling);

	// ------------------------------------------------------------

	core->freq = 0.0;
	return core;
}

// -----------------------------------------------------------------------

/* Deallocate resources */
void lingot_core_destroy(LingotCore* core) {
	pthread_attr_destroy(&core->attr);

#ifdef LIB_FFTW  
	fftw_destroy_plan(core->fftwplan);
	free(core->fftw_in);
	free(core->fftw_out);
#else
	lingot_fft_destroy_phase_factors(); // destroy phase factors.
	free(core->fft_out);
#endif

	lingot_audio_destroy(core->audio);

	free(core->spd_fft);
	free(core->X);
	free(core->spd_dft);
	free(core->diff2_spd_fft);
	free(core->read_buffer);
	free(core->flt_read_buffer);
	free(core->temporal_buffer);

	free(core->windowed_temporal_buffer);
	free(core->windowed_fft_buffer);

	free(core->hamming_window_temporal);
	free(core->hamming_window_fft);

	lingot_filter_destroy(core->antialiasing_filter);

	free(core);
}

// -----------------------------------------------------------------------

// signal decimation with antialiasing, in & out overlapables.
void lingot_core_decimate(LingotCore* core, FLT* in, FLT* out) {
	register unsigned int i, j;

	// low pass filter to avoid aliasing.
	lingot_filter_filter(core->antialiasing_filter,
			core->conf->read_buffer_size*core->conf->oversampling, in, in);

	// compression.
	for (i = j = 0; i < core->conf->read_buffer_size; i++, j
			+= core->conf->oversampling)
		out[i] = in[j];
}

// -----------------------------------------------------------------------

void lingot_core_process(LingotCore* core) {
	register unsigned int i, k; // loop variables.
	FLT delta_w_FFT = 2.0*M_PI/core->conf->fft_size;
	// FFT resolution in rads.  

# ifdef LIBSNDOBJ
	audio->Read();
	for (i = 0; i < core->conf->oversampling*core->conf->read_buffer_size; i++)
	flt_read_buffer[i] = audio->Output(i);
# else
	if ((lingot_audio_read(core->audio, core->read_buffer,
			core->conf->oversampling*core->conf->read_buffer_size
					*sizeof(SAMPLE_TYPE)))< 0) {
		//perror("Error reading samples");
		return;
	}

	for (i = 0; i < core->conf->oversampling*core->conf->read_buffer_size; i++)
		core->flt_read_buffer[i] = core->read_buffer[i];
# endif

	//
	// just readed:
	//
	//  ----------------------------
	// |bxxxbxxxbxxxbxxxbxxxbxxxbxxx|
	//  ----------------------------
	//
	// <----------------------------> read_buffer_size*oversampling
	//

	/* we shift the temporal window to leave a hollow where place the new piece
	 of data read. The buffer is actually a queue. */
	if (core->conf->temporal_buffer_size > core->conf->read_buffer_size)
		memcpy(core->temporal_buffer,
				&core->temporal_buffer[core->conf->read_buffer_size],
				(core->conf->temporal_buffer_size
						- core->conf->read_buffer_size) *sizeof(FLT));

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

	/* we decimate the read signal and put it at the end of the buffer. */
	if (core->conf->oversampling > 1)
		lingot_core_decimate(
				core,
				core->flt_read_buffer,
				&core->temporal_buffer[core->conf->temporal_buffer_size - core->conf->read_buffer_size]);
	else
		memcpy(
				&core->temporal_buffer[core->conf->temporal_buffer_size - core->conf->read_buffer_size],
				core->flt_read_buffer, core->conf->read_buffer_size*sizeof(FLT));
	//
	//  ------------------------------------------
	// | xxxxxxxxxxxxxxxxyyyyaa | aaaaa | bbbbbbb |
	//  ------------------------------------------ 
	//

	// ----------------- TRANSFORMATION TO FREQUENCY DOMAIN ----------------

	FLT _1_N2 = 1.0/(core->conf->fft_size*core->conf->fft_size);
	// SPD normalization constant

	// windowing
	for (i = 0; i < core->conf->fft_size; i++) {
		core->windowed_fft_buffer[i]
				= core->temporal_buffer[core->conf->temporal_buffer_size - core->conf->fft_size + i]
						*core->hamming_window_fft[i];
	}

# ifdef LIB_FFTW
	for (i = 0; i < core->conf->fft_size; i++)
	fftw_in[i].re = core->windowed_fft_buffer[i];

	// transformation.
	fftw_one(core->fftwplan, core->fftw_in, core->fftw_out);

	// esteem of SPD from FFT. (normalized squared module)
	for (i = 0; i < ((core->conf->fft_size > 256) ? (core->conf->fft_size >> 1) : 256); i++)
	core->spd_fft[i] = (core->fftw_out[i].re*core->fftw_out[i].re +
			core->fftw_out[i].im*core->fftw_out[i].im)*_1_N2;
# else

	// transformation.
	lingot_fft_fft(core->windowed_fft_buffer, core->fft_out,
			core->conf->fft_size);

	// esteem of SPD from FFT. (normalized squared module)
	for (i = 0; i < ((core->conf->fft_size > 256) ? (core->conf->fft_size >> 1)
			: 256); i++)
		core->spd_fft[i] = (core->fft_out[i].r*core->fft_out[i].r
				+ core->fft_out[i].i*core->fft_out[i].i)*_1_N2;
# endif

	// representable piece
	memcpy(core->X, core->spd_fft,
			((core->conf->fft_size > 256) ? (core->conf->fft_size >> 1) : 256)
					*sizeof(FLT));

	// truncated 2nd derivative esteem, to enhance peaks
	core->diff2_spd_fft[0] = 0.0;
	for (i = 1; i < (core->conf->fft_size >> 1) - 1; i++) {
		core->diff2_spd_fft[i] = 2.0*core->spd_fft[i]- core->spd_fft[i - 1]
				- core->spd_fft[i + 1]; // centred 2nd order derivative, to avoid group delay.
		if (core->diff2_spd_fft[i] < 0.0)
			core->diff2_spd_fft[i] = 0.0; // truncation
	}

	// peaks searching in that signal.
	int Mi = lingot_signal_get_fundamental_peak(core->conf, core->spd_fft,
			core->diff2_spd_fft, (core->conf->fft_size >> 1)); // take the fundamental peak.

	if (Mi == (signed) (core->conf->fft_size >> 1)) {
		core->freq = 0.0;
		return;
	}

	FLT w = (Mi - 1)*delta_w_FFT;
	// frecuencia de la muestra anterior al pico.

	//  Approximation to fundamental frequency by selective DFTs
	// ---------------------------------------------------------

	FLT d_w = delta_w_FFT;
	for (k = 0; k < core->conf->dft_number; k++) {

		d_w = 2.0*d_w/(core->conf->dft_size - 1); // resolution in rads.

		if (k == 0) {
			lingot_fft_spd(core->windowed_fft_buffer, core->conf->fft_size, w
					+ d_w, d_w, &core->spd_dft[1], core->conf->dft_size- 2);
			core->spd_dft[0] = core->spd_fft[Mi - 1];
			core->spd_dft[core->conf->dft_size - 1] = core->spd_fft[Mi + 1]; // 2 samples known.
		} else
			lingot_fft_spd(core->windowed_fft_buffer, core->conf->fft_size, w,
					d_w, core->spd_dft, core->conf->dft_size);

		lingot_signal_get_max(core->spd_dft, core->conf->dft_size, &Mi); // search the maximum.

		w += (Mi - 1)*d_w; // previous sample to the peak.
	}

	w += d_w; // approximation by DFTs.

	// windowing
	for (i = 0; i < core->conf->temporal_buffer_size; i++) {
		core->windowed_temporal_buffer[i] = core->temporal_buffer[i]
				*core->hamming_window_temporal[i];
	}

	//  Maximum finding by Newton-Raphson
	// -----------------------------------

	FLT wk = -1.0e5;
	FLT wkm1 = w;
	// first iterator set to the current approximation.
	FLT d1_SPD, d2_SPD;

	for (k = 0; (k < core->conf->max_nr_iter) && (fabs(wk - wkm1) > 1.0e-8); k++) {
		wk = wkm1;

		// ! we use the WHOLE temporal window for greater precission.
		lingot_fft_spd_diffs(core->windowed_temporal_buffer,
				core->conf->temporal_buffer_size, wk, &d1_SPD, &d2_SPD);
		//printf("%f %f %f\n", wk, d1_SPD, d2_SPD);
		wkm1 = wk - d1_SPD/d2_SPD;
	}

	w = wkm1; // frequency in rads.
	core->freq = (w*core->conf->sample_rate) /(2.0*M_PI
			*core->conf->oversampling); // analog frequency.

}

/* start running the core in another thread */
void lingot_core_start(LingotCore* core) {
	pthread_attr_init(&core->attr);

	// detached thread.
	//  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&core->thread, &core->attr, (void* (*)(void*)) lingot_core_run, core);
}

/* stop running the core */
void lingot_core_stop(LingotCore* core) {
	void* thread_result;

	core->running = 0;

	// wait for the thread exit
	pthread_join(core->thread, &thread_result);
}

/* run the core */
void lingot_core_run(LingotCore* core) {
	while (core->running) {
		lingot_core_process(core); // process new data block.
	}
}
