#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "lingot-complex.h"
#include "lingot-complex.c"
#include "lingot-filter.h"
#include "lingot-filter.c"
#include "lingot-signal.h"
#include "lingot-signal.c"

int lingot_signal_test() {

	const int N = 16;
	int i = 0;
	FLT* spd = malloc(N * sizeof(FLT));
	FLT* noise = malloc(N * sizeof(FLT));

	for (i = 0; i < N; i++) {
		spd[i] = i + 1;
		noise[i] = -1.0;
	}

	lingot_signal_compute_noise_level(spd, N, 5, noise);

	printf("S = [");
	for (i = 0; i < N; i++) {
		printf(" %f ", spd[i]);
	}
	printf("] \n");
	printf("N = [");
	for (i = 0; i < N; i++) {
		printf(" %f ", noise[i]);
	}
	printf("] \n");

	puts("done.");

	printf("S = [");
	for (i = 0; i < N; i++) {
		printf(" %f ", spd[i]);
	}
	printf("] \n");
	printf("median = %f\n", lingot_signal_quick_select(spd, 3));
	printf("S = [");
	for (i = 0; i < N; i++) {
		printf(" %f ", spd[i]);
	}
	printf("] \n");

	free(spd);
	free(noise);

	return 0;
}
