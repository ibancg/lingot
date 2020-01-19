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

#ifndef LINGOT_MSG_H
#define LINGOT_MSG_H

// asynchronous message handling (thread-safe)

#ifdef __cplusplus
extern "C" {
#endif

// maximum size per message
#define LINGOT_MSG_MAX_SIZE 1000

// message types
typedef enum {
    LINGOT_MSG_ERROR = 0,
    LINGOT_MSG_WARNING = 1,
    LINGOT_MSG_INFO = 2,
    LINGOT_MSG_DEBUG = 3
} lingot_msg_type_t;

// add messages to the queue
void lingot_msg_add_error(const char* message);
void lingot_msg_add_error_with_code(const char* message, int error_code);
void lingot_msg_add_warning(const char* message);
void lingot_msg_add_info(const char* message);

void lingot_msg_add(lingot_msg_type_t type, const char* message);
void lingot_msg_add_with_code(lingot_msg_type_t type, const char* message, int error_code);

// gets a message from the queue, it returns 0 if no messages are available
int lingot_msg_pop(char* dst, lingot_msg_type_t* type, int* error_code);

#ifdef __cplusplus
}
#endif

#endif
