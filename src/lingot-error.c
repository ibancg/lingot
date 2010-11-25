/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2010  Ibán Cereijo Graña, Jairo Chapela Martínez.
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
#include <string.h>
#include <pthread.h>

#include "lingot-error.h"

#define ERROR_QUEUE_SIZE 10

char message[ERROR_QUEUE_SIZE][1000];
int top = 0;

pthread_mutex_t error_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

// TODO: implement a queue, not a stack

void lingot_error_queue_push(const char* msg) {

	if (top < ERROR_QUEUE_SIZE) {
		// search if the message is already in the queue
		pthread_mutex_lock(&error_queue_mutex);

		int i;
		for (i = 0; i < top; i++) {
			if (!strcmp(message[i], msg)) {
				pthread_mutex_unlock(&error_queue_mutex);
				return;
			}
		}

		strcpy(message[top++], msg);
		pthread_mutex_unlock(&error_queue_mutex);
	} else {
		printf("WARNING: the queue is full!\n");
	}
}

char* lingot_error_queue_pop() {
	char* result = NULL;

	if (top != 0) {
		pthread_mutex_lock(&error_queue_mutex);
		result = strdup(message[--top]);
		pthread_mutex_unlock(&error_queue_mutex);
	}

	return result;
}
