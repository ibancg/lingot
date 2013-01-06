/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2013  Ibán Cereijo Graña, Jairo Chapela Martínez.
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

#define MAX_MESSAGES 	5

char message[MAX_MESSAGES][1000];
message_type_t message_type[MAX_MESSAGES];
int error_codes[MAX_MESSAGES];

int front = 0, rear = 0;

pthread_mutex_t message_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

void lingot_msg_add_error(const char* msg) {
	lingot_msg_add(msg, ERROR, 0);
}

void lingot_msg_add_error_with_code(const char* msg, int error_code) {
	lingot_msg_add(msg, ERROR, error_code);
}

void lingot_msg_add_warning(const char* msg) {
	lingot_msg_add(msg, WARNING, 0);
}

void lingot_msg_add_info(const char* msg) {
	lingot_msg_add(msg, INFO, 0);
}

void lingot_msg_add(const char* msg, message_type_t type, int error_code) {

	pthread_mutex_lock(&message_queue_mutex);
	if (front == ((rear + 1) % MAX_MESSAGES)) {
		fprintf(stderr, "warning: the messages queue is full!\n");
	} else {
		// check if the message is already in the queue
		int duplicated = 0;
		int i = front;
		while (i != rear) {
			i = (i + 1) % MAX_MESSAGES;
			if (!strcmp(message[i], msg)) {
				duplicated = 1;
				fprintf(stderr, "warning: duplicated message: %s\n", msg);
				break;
			}
		}

		if (!duplicated) {
			rear = ((rear + 1) % MAX_MESSAGES);
			strcpy(message[rear], msg);
			message_type[rear] = type;
			error_codes[rear] = error_code;

			if (type != INFO) {
				fprintf(stderr, "%s: %s\n",
						(message_type == ERROR) ? "error" : "warning", msg);
					}
				}
			}
	pthread_mutex_unlock(&message_queue_mutex);
}

int lingot_msg_get(char** msg, message_type_t* type, int* error_code) {
	int result = 0;
	*msg = NULL;

	pthread_mutex_lock(&message_queue_mutex);
	if (front != rear) {
		front = (front + 1) % MAX_MESSAGES;
		*msg = strdup(message[front]);
		*type = message_type[front];
		*error_code = error_codes[front];
		result = 1;
	}
	pthread_mutex_unlock(&message_queue_mutex);

	return result;
}
