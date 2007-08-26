//-*- C++ -*-
/*
 * filter, a musical instrument tuner.
 *
 * Copyright (C) 2004-2007  Ibán Cereijo Graña, Jairo Chapela Martínez.
 *
 * This file is part of filter.
 *
 * filter is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * filter is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with filter; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <memory.h>
#include "lingot-filter.h"

#define max(a,b) (((a)<(b))?(b):(a))

/*LingotFilter* lingot_filter_new()
  {
    LingotFilter* filter = new LingotFilter;
    filter->N = 0;
    filter->a = NULL;
    filter->b = NULL;
    filter->s = NULL;
    return filter;
  }*/

// given each polynomial order and coefs, with optional initial status.
LingotFilter* lingot_filter_new(unsigned int Na, unsigned int Nb, FLT* a,
    FLT* b)
  {
    unsigned int i;
    LingotFilter* filter = malloc(sizeof(LingotFilter));
    filter->N = max(Na, Nb);

    filter->a = malloc((filter->N + 1)*sizeof(FLT));
    filter->b = malloc((filter->N + 1)*sizeof(FLT));
    filter->s = malloc((filter->N + 1)*sizeof(FLT));

    for (i = 0; i < filter->N + 1; i++)
      filter->a[i] = filter->b[i] = filter->s[i] = 0.0;

    memcpy(filter->a, a, (Na + 1)*sizeof(FLT));
    memcpy(filter->b, b, (Nb + 1)*sizeof(FLT));

    for (i = 0; i < filter->N + 1; i++)
      {
        filter->a[i] /= a[0]; // polynomial normalization.
        filter->b[i] /= a[0];
      }

    return filter;
  }

void lingot_filter_destroy(LingotFilter* filter)
  {
    free(filter->a);
    free(filter->b);
    free(filter->s);

    free(filter);
  }

// Digital Filter Implementation II, in & out overlapables.
void lingot_filter_filter(LingotFilter* filter, unsigned int n, FLT* in,
    FLT* out)
  {
    FLT w, y;
    register unsigned int i;
    register int j;

    for (i = 0; i < n; i++)
      {

        w = in[i];
        y = 0.0;

        for (j = filter->N - 1; j >= 0; j--)
          {
            w -= filter->a[j + 1]*filter->s[j];
            y += filter->b[j + 1]*filter->s[j];
            filter->s[j + 1] = filter->s[j];
          }

        y += w*filter->b[0];
        filter->s[0] = w;

        out[i] = y;
      }
  }

// single sample filtering
FLT lingot_filter_filter_sample(LingotFilter* filter, FLT in)
  {
    FLT result;

    lingot_filter_filter(filter, 1, &in, &result);
    return result;
  }

