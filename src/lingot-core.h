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

#ifndef LINGOT_CORE_H
#define LINGOT_CORE_H

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "lingot-defs.h"
#include "lingot-config.h"

typedef struct {
    void *core_private;
} lingot_core_t;

//----------------------------------------------------------------

void lingot_core_new(lingot_core_t*, lingot_config_t*);
void lingot_core_destroy(lingot_core_t*);

// runs the core mainloop
void lingot_core_mainloop(lingot_core_t* core);

void lingot_core_compute_fundamental_fequency(lingot_core_t* core);

// starts the core in the calling thread (starts the audio thread)
void lingot_core_start(lingot_core_t*);

// stops the core (audio thread)
void lingot_core_stop(lingot_core_t*);

// starts the core in another thread
void lingot_core_thread_start(lingot_core_t*);

// stops the core started in another thread
void lingot_core_thread_stop(lingot_core_t*);

// Thread-safe request if the core is running
int lingot_core_thread_is_running(const lingot_core_t*);

// Thread-safe request to the latest computed frequency
FLT lingot_core_thread_get_result_frequency(lingot_core_t*);

// Thread-safe access to the latest computed spectrum. The SPD is copied into
// an internal secondary buffer that can be accessed afterwards from the calling thread
FLT* lingot_core_thread_get_result_spd(lingot_core_t*);

#ifdef __cplusplus
}
#endif

#endif //LINGOT_CORE_H
