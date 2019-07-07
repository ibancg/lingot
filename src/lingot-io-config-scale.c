/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2019  Iban Cereijo.
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
#include <errno.h>
#include <math.h>

#include "lingot-io-config-scale.h"
#include "lingot-i18n.h"
#include "lingot-msg.h"

int lingot_config_scale_parse_shift(char* char_buffer, double* cents,
                                    short int* numerator, short int* denominator) {
    static const char* delim = "/";
    char* char_buffer_pointer1 = strtok(char_buffer, delim);
    char* char_buffer_pointer2 = strtok(NULL, delim);
    short int num, den;
    int result = 1;

    if (numerator != NULL) {
        *numerator = -1;
    }

    if (denominator != NULL) {
        *denominator = -1;
    }

    int n = 0;
    if (!char_buffer_pointer2) {
        if (!char_buffer_pointer1) {
            result = 0;
        } else {
            n = sscanf(char_buffer_pointer1, "%lf", cents);
            if (!n) {
                result = 0;
            }
        }
    } else {
        n = sscanf(char_buffer_pointer1, "%hd", &num);
        if (!n) {
            result = 0;
        } else {
            n = sscanf(char_buffer_pointer2, "%hd", &den);
            if (!n) {
                result = 0;
            } else {
                *cents = 1200.0 * log2(1.0 * num / den);
                if (numerator != NULL) {
                    *numerator = num;
                }
                if (denominator != NULL) {
                    *denominator = den;
                }
            }
        }
    }

    if (!result) {
        *numerator = 1;
        *denominator = 1;
        *cents = 0.0;
    }

    return result;
}

void lingot_config_scale_format_shift(char* char_buffer, double cents,
                                      short int numerator, short int denominator) {
    if (numerator < 0) {
        sprintf(char_buffer, "%0.4lf", cents);
    } else {
        sprintf(char_buffer, "%hd/%hd", numerator, denominator);
    }
}

int lingot_config_scale_load_scl(LingotScale* scale, char* filename) {
    FILE* fp;
    int i;
    char* char_buffer_pointer1;
    char* nl;
    static const char* delim = " \t\n";
    int result = 1;
    int line = 0;
    const char* error_format_msg = _("incorrect format");
    const char* error_note_number_msg = _("note number mismatch");
    const char* exception;

#   define MAX_LINE_SIZE 1000

    char char_buffer[MAX_LINE_SIZE];

    if ((fp = fopen(filename, "r")) == NULL) {
        char buff[1000];
        snprintf(buff, sizeof(buff), "%s\n%s", _("Error opening scale file"),
                 strerror(errno));
        lingot_msg_add_error(buff);
        return 0;
    }

    scale->base_frequency = MID_C_FREQUENCY;

    try
    {
        line++;
        if (!fgets(char_buffer, MAX_LINE_SIZE, fp)) {
            throw(error_format_msg);
        }

        if (strchr(char_buffer, '!') != char_buffer) {
            throw(error_format_msg);
        }

        line++;
        if (!fgets(char_buffer, MAX_LINE_SIZE, fp)) {
            throw(error_format_msg);
        }

        line++;
        if (!fgets(char_buffer, MAX_LINE_SIZE, fp)) {
            throw(error_format_msg);
        }

        nl = strrchr(char_buffer, '\r');
        if (nl)
            *nl = '\0';
        nl = strrchr(char_buffer, '\n');
        if (nl)
            *nl = '\0';
        scale->name = strdup(char_buffer);

        line++;
        if (!fgets(char_buffer, MAX_LINE_SIZE, fp)) {
            throw(error_format_msg);
        }
        sscanf(char_buffer, "%hu", &scale->notes);

        line++;
        if (!fgets(char_buffer, MAX_LINE_SIZE, fp)) {
            throw(error_format_msg);
        }
        lingot_config_scale_allocate(scale, scale->notes);

        scale->note_name[0] = strdup("1");
        scale->offset_cents[0] = 0.0;
        scale->offset_ratios[0][0] = 1;
        scale->offset_ratios[1][0] = 1; // 1/1

        for (i = 1; i < scale->notes; i++) {

            line++;
            if (!fgets(char_buffer, MAX_LINE_SIZE, fp)) {
                throw(error_note_number_msg);
            }

            char_buffer_pointer1 = strtok(char_buffer, delim);

            if (char_buffer_pointer1 == NULL) {
                throw(error_note_number_msg);
            }

            int r = lingot_config_scale_parse_shift(char_buffer_pointer1,
                                                    &scale->offset_cents[i], &scale->offset_ratios[0][i],
                    &scale->offset_ratios[1][i]);
            if (!r) {
                throw(error_format_msg);
            } else {
                if (scale->offset_cents[i] <= scale->offset_cents[i - 1]) {
                    throw(_("the notes must be well ordered"));
                }
            }

            sprintf(char_buffer, "%d", i + 1);
            scale->note_name[i] = strdup(char_buffer);
        }
    } catch {
        result = 0;
        char buff[1000];
        snprintf(buff, sizeof(buff), "%s, line %i: %s",
                 _("Error opening scale file"), line, exception);
        lingot_msg_add_error(buff);
    }

    fclose(fp);

#   undef MAX_LINE_SIZE
    return result;
}
