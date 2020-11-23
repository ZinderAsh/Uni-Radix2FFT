#include "fft.h"

/**
 * This file contains the implementation of `fft_compute` and ancillary
 * functions.
 */

// Include for math functions and definition of PI
#include <math.h>
// Included to get access to `malloc` and `free`
#include <stdlib.h>
// pthread for threading
#include <pthread.h>

#define TAU (2. * M_PI)

// Struct to contain args for fft_compute2 threads
struct fft_args {
	const complex* in;
	complex* out;
	int n;
	int s;
	const complex* w_cache;
};

// Struct to contain args for cache calculating threads
struct cache_args {
	complex* w_cache;
	int offset;
	int n;
};

void *fft_compute_thread(void* threadarg);

void fft_compute2(const complex* in, complex* out, const int n, const int s, const complex* w_cache) {
	const int half = n / 2;
	if (n == 2) {
        // At the lowers level, so set out <- in
		out[0] = in[0];
		out[1] = in[s];
	} else if (s > 2) {
		// Recursively calculate the result for bottom and top half
		fft_compute2(in, out, half, s * 2, w_cache + half);
		fft_compute2(in + s, out + half, half, s * 2, w_cache + half);
	} else {
        // For the first two iterations, split into threads for a total of 4 threads
		struct fft_args arg;
		arg.in = in;
		arg.out = out;
		arg.n = half;
		arg.s = s * 2;
		arg.w_cache = w_cache + half;
		pthread_t th;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		int rc = pthread_create(&th, &attr, fft_compute_thread, (void*) &arg);

		fft_compute2(in + s, out + half, half, s * 2, w_cache + half);
		pthread_join(th, NULL);

	}
	// Combine the output of the two previous recursions
	for(int i = 0; i < half; ++i) {
		const complex e = out[i];
		const complex o = out[i + half];
		const complex w = w_cache[i];
		out[i]        = e + w * o;
		out[i + half] = e - w * o;
	}
}

void *fft_compute_thread(void* threadarg) {
    // Extract args from struct and call the fft_compute2 function for the thread
	struct fft_args* args;
	args = (struct fft_args*) threadarg;
	fft_compute2(args->in, args->out, args->n, args->s, args->w_cache);
	pthread_exit(NULL);
}

// Creates one fouth of the cache, where which fourth is specified by an offset from 0-3
void fft_make_cache_part(complex* w_cache, const int n, const int offset) {
	int pos = 0;
	for (int i = n; i > 0; i /= 2) {
		for (int j = offset * i / 8; j < (offset + 1) * i / 8; j++) {
			w_cache[pos + j] = cexp(0 - (TAU * j) / i * I);
		}
		pos += i / 2;
	}
}

void *fft_cache_thread(void* threadarg) {
    // Extract args from struct and call the cache calculating function for the thread
	struct cache_args* args = (struct cache_args*) threadarg;
	fft_make_cache_part(args->w_cache, args->n, args->offset);
	pthread_exit(NULL);
}

void fft_compute(const complex* in, complex* out, const int n) {
	complex* w_cache = malloc(sizeof(complex) * n * 2);
	int pos = 0;
	if (n <= 256) {
        // In data is small, don't bother threading the caching
		int pos = 0;
		for (int i = n; i > 0; i /= 2) {
			for (int j = 0; j < i / 2; j++) {
				w_cache[pos + j] = cexp(0 - (TAU * j) / i * I);
			}
			pos += i / 2;
		}
	} else {
        // Split cache calculating into 4 threads
		pthread_t threads[3];
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		
		struct cache_args args[3];

        // Create 3 new threads
		for (int i = 0; i < 3; i++) {
			args[i].w_cache = w_cache;
			args[i].offset = i;
			args[i].n = n;
			pthread_create(&threads[i], &attr, fft_cache_thread, (void*) &args[i]);
		}

		// The original thread is the fourth
		fft_make_cache_part(w_cache, n, 3);

		for (int i = 0; i < 3; i++) {
			pthread_join(threads[i], NULL);
		}
	}
	fft_compute2(in, out, n, 1, w_cache);
	free(w_cache);
}
