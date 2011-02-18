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

#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "lingot-msg.h"

#define MAX_MESSAGES 5

char message[MAX_MESSAGES][1000];
message_type_t message_type[MAX_MESSAGES];

int front = 0, rear = 0;

pthread_mutex_t message_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

void lingot_msg_add_error(const char* msg) {
	lingot_msg_add(msg, ERROR);
}

void lingot_msg_add_warning(const char* msg) {
	lingot_msg_add(msg, WARNING);
}

void lingot_msg_add_info(const char* msg) {
	lingot_msg_add(msg, INFO);
}

void lingot_msg_add(const char* msg, message_type_t type) {

	pthread_mutex_lock(&message_queue_mutex);
	if (front == ((rear + 1) % MAX_MESSAGES)) {
		perror("The messages array is full!\n");
	} else {
		// check if the message is already in the queue
		int duplicated = 0;
		int i = front;
		while (i != rear) {
			i = (i + 1) % MAX_MESSAGES;
			if (!strcmp(message[i], msg)) {
				duplicated = 1;
				printf("duplicated message: %s\n", msg);
				break;
			}
		}

		if (!duplicated) {
			rear = ((rear + 1) % MAX_MESSAGES);
			strcpy(message[rear], msg);
			message_type[rear] = type;
		}
	}
	pthread_mutex_unlock(&message_queue_mutex);
}

int lingot_msg_get(char** msg, message_type_t* type) {
	int result = 0;
	*msg = NULL;

	pthread_mutex_lock(&message_queue_mutex);
	if (front != rear) {
		front = (front + 1) % MAX_MESSAGES;
		*msg = strdup(message[front]);
		*type = message_type[front];
		result = 1;
	}
	pthread_mutex_unlock(&message_queue_mutex);

	return result;
}
