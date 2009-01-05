
#include <stdlib.h>

#include "lingot-error.h"

char* message = NULL;

void lingot_error_queue_push(char* msg) {
	message = strdup(msg);
}

char* lingot_error_queue_pop() {
	char* result = message;
	message = NULL;
	return result;
}
