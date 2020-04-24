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

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>

#include "lingot-signal.h"
#include "lingot-filter.h"

static int lingot_signal_is_peak(const LINGOT_FLT* signal, int index, unsigned short peak_half_width) {
    int j;

    for (j = 0; j < peak_half_width; j++) {
        if ((signal[index + j] < signal[index + j + 1])
                || (signal[index - j] < signal[index - j - 1])) {
            return 0;
        }
    }
    return 1;
}

//---------------------------------------------------------------------------

static LINGOT_FLT lingot_signal_fft_bin_interpolate_quinn2_tau(LINGOT_FLT x) {
    return (0.25 * log(3 * x * x + 6 * x + 1.0)
            - 0.102062072615966
            * log(
                (x + 1.0 - 0.816496580927726)
                / (x + 1.0 + 0.816496580927726)));
}

static LINGOT_FLT lingot_signal_fft_bin_interpolate_quinn2(const LINGOT_FLT complex y1,
                                                           const LINGOT_FLT complex y2,
                                                           const LINGOT_FLT complex y3) {
    LINGOT_FLT ap = creal(y3 / y2);
    LINGOT_FLT dp = -ap / (1.0 - ap);
    LINGOT_FLT am = creal(y1 / y2);
    LINGOT_FLT dm = am / (1.0 - am);
    return 0.5 * (dp + dm)
            + lingot_signal_fft_bin_interpolate_quinn2_tau(dp * dp)
            - lingot_signal_fft_bin_interpolate_quinn2_tau(dm * dm);
}

// for qsort
static int lingot_signal_compare_int(const void *a, const void *b) {
    return (*((const int*) a) < *((const int*) b)) ? -1 : 1;
}

int lingot_signal_frequencies_related(LINGOT_FLT freq1, LINGOT_FLT freq2, LINGOT_FLT minFrequency,
                                      LINGOT_FLT* mulFreq1ToFreq, LINGOT_FLT* mulFreq2ToFreq) {

    int result = 0;
    static const LINGOT_FLT tol = 5e-2; // TODO: tune
    static const int maxDivisor = 4;

    if ((freq1 != 0.0) && (freq2 != 0.0)) {

        LINGOT_FLT smallFreq = freq1;
        LINGOT_FLT bigFreq = freq2;

        if (bigFreq < smallFreq) {
            smallFreq = freq2;
            bigFreq = freq1;
        }

        int divisor;
        LINGOT_FLT frac;
        LINGOT_FLT error = -1.0;
        for (divisor = 1; divisor <= maxDivisor; divisor++) {
            if (minFrequency * divisor > smallFreq) {
                break;
            }

            frac = bigFreq * divisor / smallFreq;
            error = fabs(frac - round(frac));
            if (error < tol) {
                if (smallFreq == freq1) {
                    *mulFreq1ToFreq = 1.0 / divisor;
                    *mulFreq2ToFreq = 1.0 / round(frac);
                } else {
                    *mulFreq1ToFreq = 1.0 / round(frac);
                    *mulFreq2ToFreq = 1.0 / divisor;
                }
                result = 1;
                break;
            }
        }
    } else {
        *mulFreq1ToFreq = 0.0;
        *mulFreq2ToFreq = 0.0;
    }

    //	printf("relation %f, %f = %i\n", freq1, freq2, result);

    return result;
}

LINGOT_FLT lingot_signal_frequency_locker(LINGOT_FLT freq, LINGOT_FLT minFrequency) {

    static int locked = 0;
    static LINGOT_FLT current_frequency = -1.0;
    static int hits_counter = 0;
    static int rehits_counter = 0;
    static int rehits_up_counter = 0;
    static const int nhits_to_lock = 4;
    static const int nhits_to_unlock = 5;
    static const int nhits_to_relock = 6;
    static const int nhits_to_relock_up = 8;
    LINGOT_FLT multiplier = 0.0;
    LINGOT_FLT multiplier2 = 0.0;
    static LINGOT_FLT old_multiplier = 0.0;
    static LINGOT_FLT old_multiplier2 = 0.0;
    int fail = 0;
    LINGOT_FLT result = 0.0;

    int consistent_with_current_frequency = 0;
    consistent_with_current_frequency = lingot_signal_frequencies_related(freq, current_frequency, minFrequency,
                                                                          &multiplier, &multiplier2);

    if (!locked) {

        if ((freq > 0.0) && (current_frequency == 0.0)) {
            consistent_with_current_frequency = 1;
            multiplier = 1.0;
            multiplier2 = 1.0;
        }

        //		printf("filtering frequency %f, current %f\n", freq, current_frequency);

        if (consistent_with_current_frequency && (multiplier == 1.0)
                && (multiplier2 == 1.0)) {
            current_frequency = freq * multiplier;

            if (++hits_counter >= nhits_to_lock) {
                locked = 1;
                hits_counter = 0;
            }
        } else {
            hits_counter = 0;
            current_frequency = 0.0;
        }

        //		result = freq;
    } else {
        //		printf("c = %i, f = %f, cf = %f, multiplier = %f, multiplier2 = %f\n",
        //				consistent_with_current_frequency, freq, current_frequency,
        //				multiplier, multiplier2);

        if (consistent_with_current_frequency) {
            if (fabs(multiplier2 - 1.0) < 1e-5) {
                result = freq * multiplier;
                current_frequency = result;
                rehits_counter = 0;

                if (fabs(multiplier - 1.0) > 1e-5) {
                    if (fabs(multiplier - old_multiplier) < 1e-5) {
                        if (++rehits_up_counter >= nhits_to_relock_up) {
                            result = freq;
                            current_frequency = result;
                            rehits_up_counter = 0;
                            fail = 0;
                        }
                    } else {
                        rehits_up_counter = 0;
                    }
                } else {
                    rehits_up_counter = 0;
                }
            } else {
                rehits_up_counter = 0;
                if (fabs(multiplier2 - 0.5) < 1e-5) {
                    hits_counter--;
                }
                fail = 1;
                if (freq * multiplier < minFrequency) {
                } else {
                    //					result = freq * multiplier;
                    //					printf("hop detected!\n");
                    //					current_frequency = result;

                    if (fabs(multiplier2 - old_multiplier2) < 1e-5) {
                        if (++rehits_counter >= nhits_to_relock) {
                            result = freq * multiplier;
                            current_frequency = result;
                            rehits_counter = 0;
                            fail = 0;
                        }
                    }
                }
            }

        } else {
            fail = 1;
        }

        if (fail) {
            result = current_frequency;
            hits_counter++;
            if (hits_counter >= nhits_to_unlock) {
                current_frequency = 0.0;
                locked = 0;
                hits_counter = 0;
                result = 0.0;
            }
        } else {
            hits_counter = 0;
        }
    }

    old_multiplier = multiplier;
    old_multiplier2 = multiplier2;

    //	if (result != 0.0)
    //		printf("result = %f\n", result);
    return result;
}


// Returns a factor to multiply with in order to give more importance to higher
// frequency harmonics. This is to give more importance to the higher divisors
// for the same selected sets.
static LINGOT_FLT lingot_signal_frequency_penalty(LINGOT_FLT freq) {
    static const LINGOT_FLT f0 = 100;
    static const LINGOT_FLT f1 = 1000;
    static const LINGOT_FLT alpha0 = 0.99;
    static const LINGOT_FLT alpha1 = 1;

    // TODO: precompute
    const LINGOT_FLT freqPenaltyA = (alpha0 - alpha1) / (f0 - f1);
    const LINGOT_FLT freqPenaltyB = -(alpha0 * f1 - f0 * alpha1) / (f0 - f1);

    return freq * freqPenaltyA + freqPenaltyB;
}

// search the fundamental peak given the SPD and its 2nd derivative
LINGOT_FLT lingot_signal_estimate_fundamental_frequency(const LINGOT_FLT* snr,
                                                        LINGOT_FLT freq,
                                                        const LINGOT_FLT complex* fft,
                                                        unsigned int N,
                                                        unsigned int n_peaks,
                                                        unsigned int lowest_index,
                                                        unsigned int highest_index,
                                                        unsigned short peak_half_width,
                                                        LINGOT_FLT delta_f_fft,
                                                        LINGOT_FLT min_snr,
                                                        LINGOT_FLT min_q,
                                                        LINGOT_FLT min_freq,
                                                        short* divisor) {
    unsigned int i, j, m;
    int p_index[n_peaks];
    LINGOT_FLT magnitude[n_peaks];

    // at this moment there are no peaks.
    for (i = 0; i < n_peaks; i++)
        p_index[i] = -1;

    if (lowest_index < peak_half_width) {
        lowest_index = peak_half_width;
    }
    if (peak_half_width + highest_index > N) {
        highest_index = N - peak_half_width;
    }

    unsigned n_found_peaks = 0;

    // I'll get the n_peaks maximum peaks with SNR above the required.
    for (i = lowest_index; i < highest_index; i++) {

        LINGOT_FLT factor = 1.0;

        if (freq != 0.0) {

            LINGOT_FLT f = i * delta_f_fft;
            if (fabs(f / freq - round(f / freq)) < 0.07) { // TODO: tune and put in conf
                factor = 1.5; // TODO: tune and put in conf
            }

        }

        LINGOT_FLT snri = snr[i] * factor;

        if ((snri > min_snr)
                && lingot_signal_is_peak(snr, (int) i, peak_half_width)) {

            // search a place in the maximums buffer, if it doesn't exists, the
            // lower maximum is candidate to be replaced.
            m = 0;				// first candidate.
            for (j = 0; j < n_peaks; j++) {
                if (p_index[j] == -1) {
                    m = j; // there is a place.
                    break;
                }

                // if there is no place, finds the smallest peak as a candidate
                // to be substituted.
                if (magnitude[j] < magnitude[m]) {
                    m = j;
                }
            }

            if (p_index[m] == -1) {
                p_index[m] = (int) i; // there is a place
                magnitude[m] = snri;
            } else if (snr[i] > snr[p_index[m]]) {
                p_index[m] = (int) i; // if greater
                magnitude[m] = snri;
            }

            if (n_found_peaks < n_peaks) {
                n_found_peaks++;
            }
        }
    }

    if (n_found_peaks == 0) {
        return 0.0;
    }

    LINGOT_FLT maximum = 0.0;

    // search the maximum peak
    for (i = 0; i < n_found_peaks; i++)
        if ((p_index[i] != -1) && (magnitude[i] > maximum)) {
            maximum = magnitude[i];
        }

    // all peaks much lower than maximum are deleted.
    unsigned delete_counter = 0;
    for (i = 0; i < n_found_peaks; i++) {
        if ((p_index[i] == -1) || (magnitude[i] < maximum - 20.0)) { // TODO: conf
            p_index[i] = (int) N; // there are available places in the buffer.
            delete_counter++;
        }
    }

    qsort(p_index, n_found_peaks, sizeof(int), &lingot_signal_compare_int);
    n_found_peaks -= delete_counter;

    LINGOT_FLT freq_interpolated[n_found_peaks];
    LINGOT_FLT delta = 0.0;
    for (i = 0; i < n_found_peaks; i++) {
        delta = lingot_signal_fft_bin_interpolate_quinn2(fft[p_index[i] - 1],
                fft[p_index[i]], fft[p_index[i] + 1]);
        freq_interpolated[i] = delta_f_fft * (p_index[i] + delta);
    }

    // maximum ratio error
    static const LINGOT_FLT ratio_tol = 0.02; // TODO: tune
    static const short max_divisor = 4;

    unsigned short tone_index = 0;
    short div = 0;
    LINGOT_FLT groundFreq = 0.0;
    LINGOT_FLT ratios[n_found_peaks];
    LINGOT_FLT error[n_found_peaks];
    short indices_related[n_found_peaks];
    unsigned short n_indices_related = 0;

    LINGOT_FLT best_q = 0.0;
    short best_indices_related[n_found_peaks];
    LINGOT_FLT best_f = 0;
    short best_divisor = 1;

    // possible ground frequencies
    for (tone_index = 0; tone_index < n_found_peaks; tone_index++) {
        for (div = 1; div <= max_divisor; div++) {
            groundFreq = freq_interpolated[tone_index] / div;
            if (groundFreq > min_freq) {
                n_indices_related = 0;
                for (i = 0; i < n_found_peaks; i++) {
                    ratios[i] = freq_interpolated[i] / groundFreq;
                    error[i] = (ratios[i] - round(ratios[i]));
                    // harmonically related frequencies
                    if (fabs(error[i]) < ratio_tol) {
                        indices_related[n_indices_related++] = (short) i;
                    }
                }

                // add the penalties for short sets and high divisors
                //				int highest_harmonic_index = n_indices_related - 1;
                int highest_harmonic_index = 0;
                LINGOT_FLT highest_harmonic_magnitude = 0.0;
                LINGOT_FLT q = 0.0;
                LINGOT_FLT f = 0.0;
                //				FLT snrsum = 0.0;
                for (i = 0; i < n_indices_related; i++) {
                    // add up contributions to the quality factor
                    q += snr[p_index[indices_related[i]]]
                            * lingot_signal_frequency_penalty(groundFreq);
                    if (snr[p_index[indices_related[i]]] > highest_harmonic_magnitude) {
                        highest_harmonic_index = (int) i;
                        highest_harmonic_magnitude = snr[p_index[indices_related[i]]];
                    }

                    //					snrsum += snr[p_index[indices_related[i]]];
                    //					short myDivisor = round(
                    //							freq_interpolated[indices_related[i]] / groundFreq);
                    //					f += snr[p_index[indices_related[i]]]
                    //							* freq_interpolated[indices_related[i]] / myDivisor;
                }

                f = freq_interpolated[indices_related[highest_harmonic_index]];

                //				f /= snrsum;

                //				printf(">>>>>>>>>< f = %f\n", f);

                //				printf(
                //						"tone = %i, divisor = %i, gf = %f, q = %f, n = %i, f = %f, ",
                //						tone, divisor, groundFreq, q, n_indices_related, f);
                //				for (i = 0; i < n_indices_related; i++) {
                //					printf("%f ", freq_interpolated[indices_related[i]]);
                //				}
                //				printf("\n");

                if (q > best_q) {
                    best_q = q;
                    memcpy(best_indices_related, indices_related,
                           n_indices_related * sizeof(short));
                    best_divisor = (short) round(f / groundFreq);
                    best_f = f;
                }

            } else {
                break;
            }
        }
    }

    //	printf("BEST selected f = %f, f* = %f, div = %i, q = %f, n = %i, ",
    //			bestF / bestDivisor, bestDivisor, bestF, bestQ,
    //			best_n_indices_related);
    //	for (i = 0; i < best_n_indices_related; i++) {
    //		printf("%f ", freq_interpolated[best_indices_related[i]]);
    //	}
    //	printf("\n");

    if ((best_f != 0.0) && (best_q < min_q)) {
        best_f = 0.0;
        //		printf("q < mq!!\n");
    }

    *divisor = best_divisor;
    return best_f;
}

void lingot_signal_compute_noise_level(const LINGOT_FLT* spd,
                                       int N,
                                       int cbuffer_size,
                                       LINGOT_FLT* noise_level) {

    // low pass IIR filter.
    const LINGOT_FLT c = 0.1;
    const LINGOT_FLT filter_a[] = { 1.0, c - 1.0 };
    const LINGOT_FLT filter_b[] = { c };
    static char initialized = 0;
    static lingot_filter_t filter;

    if (! initialized) {
        initialized = 1;
        lingot_filter_new(&filter, 1, 0, filter_a, filter_b);
    }

    lingot_filter_reset(&filter);

    lingot_filter_filter(&filter, (unsigned int) cbuffer_size, spd, noise_level);
    lingot_filter_filter(&filter, (unsigned int) N, spd, noise_level);

}

//---------------------------------------------------------------------------

// generates a N-sample window
void lingot_signal_window(int N, LINGOT_FLT* out, window_type_t window_type) {
    int i;
    switch (window_type) {
    case HANNING:
        for (i = 0; i < N; i++)
            out[i] = 0.5 * (1 - cos((2.0 * M_PI * i) / (N - 1)));
        break;
    case HAMMING:
        for (i = 0; i < N; i++)
            out[i] = 0.53836 - 0.46164 * cos((2.0 * M_PI * i) / (N - 1));
        break;
    default:
        break;
    }
}
