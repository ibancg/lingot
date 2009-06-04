#include <stdlib.h>

#include "lingot-error.h"

char message[200];
int unpop = 0;

void lingot_error_queue_push(char* msg) {
	if (unpop == 0) {
		unpop = 1;
	} else {
		printf("WARNING: previous message found %s\n", msg);
	}
	strcpy(message, msg);
}

char* lingot_error_queue_pop() {
	if (unpop) {
		unpop = 0;
		return strdup(message);
	} else {
		return NULL;
	}
}
