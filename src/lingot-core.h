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

#ifndef LINGOT_CORE_H
#define LINGOT_CORE_H

#include <pthread.h>

#include "lingot-defs.h"
#include "lingot-complex.h"
#include "lingot-filter.h"
#include "lingot-config.h"

#include "lingot-audio.h"

#include "lingot-fft.h"

typedef struct {
    void *private;
} LingotCore;

//----------------------------------------------------------------

void lingot_core_new(LingotCore*, LingotConfig*);
void lingot_core_destroy(LingotCore*);

// runs the core mainloop
void lingot_core_mainloop(LingotCore* core);

// starts the core in another thread
void lingot_core_thread_start(LingotCore*);

// stops the core started in another thread
void lingot_core_thread_stop(LingotCore*);

// Thread-safe request if the core is running
int lingot_core_thread_is_running(LingotCore*);

// Thread-safe request to the latest computed frequency
FLT lingot_core_thread_get_result_frequency(LingotCore*);

// Thread-safe access to the latest computed spectrum. The SPD is copied into
// an internal secondary buffer that can be accessed afterwards from the calling thread
FLT* lingot_core_thread_get_result_spd(LingotCore*);

#endif //LINGOT_CORE_H
