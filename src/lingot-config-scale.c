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
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "lingot-config-scale.h"
#include "lingot-i18n.h"
#include "lingot-msg.h"
#include "lingot-defs.h"

void lingot_config_scale_new(lingot_scale_t* scale) {

    scale->name = NULL;
    scale->notes = 0;
    scale->note_name = NULL;
    scale->offset_cents = NULL;
    scale->offset_ratios[0] = NULL;
    scale->offset_ratios[1] = NULL;
    scale->base_frequency = 0.0;
}

void lingot_config_scale_allocate(lingot_scale_t* scale, unsigned short int notes) {
    scale->notes = notes;
    scale->note_name = malloc(notes * sizeof(char*));
    unsigned int i = 0;
    for (i = 0; i < notes; i++) {
        scale->note_name[i] = NULL;
    }
    scale->offset_cents = malloc(notes * sizeof(FLT));
    scale->offset_ratios[0] = malloc(notes * sizeof(short int));
    scale->offset_ratios[1] = malloc(notes * sizeof(short int));
}

void lingot_config_scale_destroy(lingot_scale_t* scale) {
    unsigned short int i;

    for (i = 0; i < scale->notes; i++) {
        free(scale->note_name[i]);
    }

    free(scale->offset_cents);
    free(scale->offset_ratios[0]);
    free(scale->offset_ratios[1]);
    free(scale->note_name);
    free(scale->name);

    lingot_config_scale_new(scale);
}

void lingot_config_scale_restore_default_values(lingot_scale_t* scale) {

    unsigned short int i;
    const char* tone_string[] = {
        _("C"), _("C#"), _("D"), _("D#"), _("E"), _("F"),
        _("F#"), _("G"), _("G#"), _("A"), _("A#"), _("B"),
    };

    lingot_config_scale_destroy(scale);

    // default 12 tones equal-tempered scale hard-coded
    scale->name = strdup(_("Default equal-tempered scale"));
    lingot_config_scale_allocate(scale, 12);

    scale->base_frequency = MID_C_FREQUENCY;

    scale->note_name[0] = strdup(tone_string[0]);
    scale->offset_cents[0] = 0.0;
    scale->offset_ratios[0][0] = 1;
    scale->offset_ratios[1][0] = 1; // 1/1

    for (i = 1; i < scale->notes; i++) {
        scale->note_name[i] = strdup(tone_string[i]);
        scale->offset_cents[i] = 100.0 * i;
        scale->offset_ratios[0][i] = -1; // not used
        scale->offset_ratios[1][i] = -1; // not used
    }
}

void lingot_config_scale_copy(lingot_scale_t* dst, const lingot_scale_t* src) {
    unsigned short int i;

    lingot_config_scale_destroy(dst);

    *dst = *src;

    dst->name = strdup(src->name);
    lingot_config_scale_allocate(dst, dst->notes);

    for (i = 0; i < dst->notes; i++) {
        dst->note_name[i] = strdup(src->note_name[i]);
        dst->offset_cents[i] = src->offset_cents[i];
        dst->offset_ratios[0][i] = src->offset_ratios[0][i];
        dst->offset_ratios[1][i] = src->offset_ratios[1][i];
    }
}

int lingot_config_scale_get_octave(const lingot_scale_t* scale, int index) {
    return (index < 0) ?
                ((index + 1) / scale->notes) - 1 :
                index / scale->notes;
}

int lingot_config_scale_get_note_index(const lingot_scale_t* scale, int index) {
    int r = index % scale->notes;
    return r < 0 ? r + scale->notes : r;
}

FLT lingot_config_scale_get_absolute_offset(const lingot_scale_t* scale, int index) {
    return lingot_config_scale_get_octave(scale, index) * 1200.0
            + scale->offset_cents[lingot_config_scale_get_note_index(scale, index)];
}

FLT lingot_config_scale_get_frequency(const lingot_scale_t* scale, int index) {
    return scale->base_frequency
            * pow(2.0,
                  lingot_config_scale_get_absolute_offset(scale, index)
                  / 1200.0);
}

int lingot_config_scale_get_closest_note_index(const lingot_scale_t* scale,
                                               FLT freq, FLT deviation, FLT* error_cents) {

    int note_index = 0;
    int index;

    FLT offset = 1200.0 * log2(freq / scale->base_frequency) - deviation;
    int octave = (int) floor(offset / 1200);
    offset = fmod(offset, 1200.0);
    if (offset < 0.0) {
        offset += 1200.0;
    }

    index = (int) floor(scale->notes * offset / 1200.0);

    // TODO: bisection?, avoid loop?
    FLT pitch_inf;
    FLT pitch_sup;
    int n = 0;
    for (;;) {
        n++;
        pitch_inf = scale->offset_cents[index];
        pitch_sup =
                ((index + 1) < scale->notes) ?
                    scale->offset_cents[index + 1] : 1200.0;

        if (offset > pitch_sup) {
            index++;
            continue;
        }

        if (offset < pitch_inf) {
            index--;
            continue;
        }

        break;
    }

    if (fabs(offset - pitch_inf) < fabs(offset - pitch_sup)) {
        note_index = index;
        *error_cents = offset - pitch_inf;
    } else {
        note_index = index + 1;
        *error_cents = offset - pitch_sup;
    }

    if (note_index == scale->notes) {
        note_index = 0;
        octave++;
    }

    note_index += octave * scale->notes;

    return note_index;
}
