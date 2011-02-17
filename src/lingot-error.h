/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2011  Ibán Cereijo Graña, Jairo Chapela Martínez.
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

#ifndef __LINGOT_ERROR_H__
#define __LINGOT_ERROR_H__

typedef enum message_type_t {
	ERROR = 0, WARNING = 1, INFO = 2
} message_type_t;

void lingot_error_queue_push(const char* message, message_type_t type);
void lingot_error_queue_push_error(const char* message);
void lingot_error_queue_push_warning(const char* message);
void lingot_error_queue_push_info(const char* message);

// pops a message from the queue, it returns 0 if no messages are available
int lingot_error_queue_pop(char** msg, message_type_t* type);

#endif
