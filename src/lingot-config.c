//-*- C++ -*-
/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2008  Ibán Cereijo Graña, Jairo Chapela Martínez.
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <locale.h>

#include "lingot-config.h"
#include "lingot-mainframe.h"

// the following tokens will appear in the config file.
char* token[] =
  { "AUDIO_DEV", "SAMPLE_RATE", "OVERSAMPLING", "ROOT_FREQUENCY_ERROR",
      "MIN_FREQUENCY", "FFT_SIZE", "TEMPORAL_WINDOW", "NOISE_THRESHOLD",
      "CALCULATION_RATE", "VISUALIZATION_RATE", "PEAK_NUMBER", "PEAK_ORDER",
      "PEAK_REJECTION_RELATION", "DFT_NUMBER", "DFT_SIZE", NULL // NULL terminated array
    };

// print/scan param formats.
const char* format = "sddffdffffddfdd";

//----------------------------------------------------------------------------

LingotConfig* lingot_config_new()
  {

    LingotConfig* config = malloc(sizeof(LingotConfig));

    lingot_config_reset(config); // set default values.

    // internal parameters mapped to each token in the config file.
    void* c_param[] =
      { &config->audio_dev, &config->sample_rate, &config->oversampling,
          &config->root_frequency_error, &config->min_frequency,
          &config->fft_size, &config->temporal_window,
          &config->noise_threshold_db, &config->calculation_rate,
          &config->visualization_rate, &config->peak_number,
          &config->peak_order, &config->peak_rejection_relation_db,
          &config->dft_number, &config->dft_size };

    memcpy(config->param, c_param, 15*sizeof(void*));

    return config;
  }

void lingot_config_destroy(LingotConfig* config)
  {
    free(config);
  }

//----------------------------------------------------------------------------
void lingot_config_reset(LingotConfig* config)
  {
    sprintf(config->audio_dev, "%s", "/dev/dsp");

    config->sample_rate = 11025; // Hz
    config->oversampling = 5;
    config->root_frequency_error = 0; // Hz
    config->min_frequency = 15; // Hz
    config->fft_size = 512; // samples
    config->temporal_window = 0.32; // seconds
    config->calculation_rate = 20; // Hz
    config->visualization_rate = 30; // Hz
    config->noise_threshold_db = 20.0; // dB

    config->peak_number = 3; // peaks
    config->peak_order = 1; // samples
    config->peak_rejection_relation_db = 20; // dB

    config->dft_number = 2; // DFTs
    config->dft_size = 15; // samples

    config->max_nr_iter = 10; // iterations

    //--------------------------------------------------------------------------

    config->vr = -0.45; // near to minimum

    lingot_config_update_internal_params(config);
  }

//----------------------------------------------------------------------------

int lingot_config_update_internal_params(LingotConfig* config)
  {
    int result = 1;

    // derived parameters.
    config->root_frequency = 440.0
        *pow(2.0, config->root_frequency_error/1200.0);
    config->temporal_buffer_size = (unsigned int)ceil(config->temporal_window
        *config->sample_rate/config->oversampling);
    config->read_buffer_size = (unsigned int)ceil(config->sample_rate
        /(config->calculation_rate*config->oversampling));
    config->peak_rejection_relation_nu = pow(10.0,
        config->peak_rejection_relation_db/10.0);
    config->noise_threshold_nu = pow(10.0, config->noise_threshold_db/10.0);

    if (config->temporal_buffer_size < config->fft_size)
      {
        config->temporal_window = ((double) config->fft_size
            *config->oversampling)/config->sample_rate;
        config->temporal_buffer_size = config->fft_size;
        result = 0;
      }

    return result;
  }

//----------------------------------------------------------------------------

void lingot_config_save(LingotConfig* config, char* filename)
  {
    unsigned int i;
    FILE* fp;
    char* lc_all;

    lc_all = setlocale(LC_ALL, NULL);
// duplicate the string, as the next call to setlocale will destroy it
    if (lc_all) lc_all = strdup(lc_all);
    setlocale(LC_ALL, "C");

    if ((fp = fopen(filename, "w")) == NULL)
      {
        char buff[100];
        sprintf(buff, "error saving config file %s ", filename);
        perror(buff);
        return;
      }

    fprintf(fp, "# Config file automatically created by lingot\n\n");

    for (i = 0; token[i]; i++)
      switch (format[i])
        {
        case 's':
          fprintf(fp, "%s = %s\n", token[i], (char*) config->param[i]);
          break;
        case 'd':
          fprintf(fp, "%s = %d\n", token[i],
              *((unsigned int*) config->param[i]));
          break;
        case 'f':
          fprintf(fp, "%s = %0.3f\n", token[i], *((FLT*) config->param[i]));
          break;
        }

    fclose(fp);

    if (lc_all) {
        setlocale(LC_ALL, lc_all);
        free(lc_all);
    }
  }

//----------------------------------------------------------------------------

void lingot_config_load(LingotConfig* config, char* filename)
  {
    FILE* fp;
    float aux;
    int line;
    int token_index;
    char* char_buffer_pointer;

    const static char* delim = " \t=\n";
    
#   define MAX_LINE_SIZE 100

    char char_buffer[MAX_LINE_SIZE];

    if ((fp = fopen(filename, "r")) == NULL)
      {
        sprintf(char_buffer,
            "error opening config file %s, assuming default values ", filename);
        perror(char_buffer);
        return;
      }

    line = 0;

    for (;;)
      {

        line++;

        if (!fgets(char_buffer, MAX_LINE_SIZE, fp))
        break;;

        //    printf("line %d: %s\n", line, s1);

        if (char_buffer[0] == '#')
        continue;

        // tokens into the line.
        char_buffer_pointer = strtok(char_buffer, delim);

        if (!char_buffer_pointer)
        continue; // blank line.

        for (token_index = 0; token[token_index]; token_index++)
          {
            if (!strcmp(char_buffer_pointer, token[token_index]))
              {
                break; // found token.
              }
          }

        if (!token[token_index])
          {
            fprintf(stderr,
                "warning: parse error at line %i: unknown keyword %s\n",
                line, char_buffer_pointer);
            continue;
          }

        // take the attribute value.
        char_buffer_pointer = strtok(NULL, delim);

        if (!char_buffer_pointer)
          {
            fprintf(stderr, "warning: parse error at line %i: value expected\n",
                line);
            continue;
          }

        // asign the value to the parameter.
        switch (format[token_index])
          {
            case 's':
            sprintf(((char*) config->param[token_index]), "%s",
                char_buffer_pointer);
            break;
            case 'd':
            sscanf(char_buffer_pointer, "%d",
                (unsigned int*) config->param[token_index]);
            break;
            case 'f':
            sscanf(char_buffer_pointer, "%f", &aux);
            *((FLT*) config->param[token_index]) = aux;
            break;
          }

      }

    fclose(fp);

    lingot_config_update_internal_params(config);

#   undef MAX_LINE_SIZE
  }
