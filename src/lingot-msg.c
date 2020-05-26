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
 * along with lingot; if not, write to the Free Software Foundation,
 * Inc. 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "lingot-msg.h"

#define MAX_MESSAGES        10

typedef struct {
    lingot_msg_type_t type;
    char message[LINGOT_MSG_MAX_SIZE + 1];
    int error_code;
} lingot_msg_t;

// message queue
lingot_msg_t message[MAX_MESSAGES];
int front = 0, rear = 0;

pthread_mutex_t message_queue_mutex = PTHREAD_MUTEX_INITIALIZER;


void lingot_msg_add_error(const char *message)
{
    lingot_msg_add_with_code(LINGOT_MSG_ERROR, message, 0);
}

void lingot_msg_add_error_with_code(const char *message, int error_code)
{
    lingot_msg_add_with_code(LINGOT_MSG_ERROR, message, error_code);
}

void lingot_msg_add_warning(const char *message)
{
    lingot_msg_add_with_code(LINGOT_MSG_WARNING, message, 0);
}

void lingot_msg_add_info(const char *message)
{
    lingot_msg_add_with_code(LINGOT_MSG_INFO, message, 0);
}

void lingot_msg_add(lingot_msg_type_t type, const char* msg) {
    lingot_msg_add_with_code(type, msg, 0);
}

void lingot_msg_add_with_code(lingot_msg_type_t type, const char* msg, int error_code) {

    pthread_mutex_lock(&message_queue_mutex);
    if (front == ((rear + 1) % MAX_MESSAGES)) {
        fprintf(stderr, "Warning: the messages queue is full!\n");
    } else {
        // check if the message is already in the queue
        int duplicated = 0;
        int i = front;
        size_t len = strlen(msg);
        if (len > LINGOT_MSG_MAX_SIZE) {
            fprintf(stderr, "Warning: message too long. Truncating!\n");
        }

        while (i != rear) {
            i = (i + 1) % MAX_MESSAGES;
            if (!strncmp(message[i].message, msg, LINGOT_MSG_MAX_SIZE)) {
                duplicated = 1;
                fprintf(stderr, "Warning: duplicated message: %s\n", msg);
                break;
            }
        }

        if (!duplicated) {
            rear = ((rear + 1) % MAX_MESSAGES);
            lingot_msg_t* msg_ = &message[rear];
            msg_->message[0] = 0;
            strncat(msg_->message, msg, LINGOT_MSG_MAX_SIZE);
            msg_->type = type;
            msg_->error_code = error_code;

            if (type != LINGOT_MSG_INFO) {
                fprintf(stderr, "%s: %s\n",
                        (type == LINGOT_MSG_ERROR) ? "Error" : "Warning", msg);
            }
        }
    }
    pthread_mutex_unlock(&message_queue_mutex);
}

int lingot_msg_pop(char *dst, lingot_msg_type_t* type, int* error_code) {
    int result = 0;

    pthread_mutex_lock(&message_queue_mutex);
    if (front != rear) {
        front = (front + 1) % MAX_MESSAGES;
        dst[0] = 0;
        lingot_msg_t* msg_ = &message[front];
        strncat(dst, msg_->message, LINGOT_MSG_MAX_SIZE);
        *type = msg_->type;
        *error_code = msg_->error_code;
        result = 1;
    }
    pthread_mutex_unlock(&message_queue_mutex);

    return result;
}
