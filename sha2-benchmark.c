/*
 * LTTng-UST benchmarks
 * Copyright (C) 2013  Samuel Demers <samuel@samdemers.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>

#include "sha2.h"
#include "shared_events.h"

#ifdef WITH_UST
#include <lttng/tracepoint.h>
#include "message.tp.h"
#endif

#define BUFFER_SIZE	(1<<10)
#define HASH_SIZE	32

/*
 * input_buffer is updated within the loop so the compiler cannot assume
 * it is unchanged between loops.
 */
static unsigned char input_buffer[BUFFER_SIZE];

static
void print_usage(int argc, char **argv)
{
	fprintf(stderr, "Usage: %s [<iterations>]\n", argv[0]);
}

static
void update_buffer(unsigned char *buffer, size_t len, unsigned int v)
{
	unsigned int i;

	for (i = 0; i < len; i++) {
		input_buffer[i] = v ^ i;
	}
}

int main(int argc, char **argv)
{
	unsigned char hash[HASH_SIZE];
	long iterations;
	sha2_context ctx;
	unsigned int i;

	if (shared_events_add("main")) {
		exit(EXIT_FAILURE);
	}

	if (argc < 2) {
		print_usage(argc, argv);
		exit(EXIT_FAILURE);
	}

	iterations = strtol(argv[1], (char **) NULL, 10);
	if (iterations <= 0) {
		print_usage(argc, argv);
		exit(EXIT_FAILURE);
	}

	sha2_starts(&ctx, 0);

	if (shared_events_add("start")) {
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < iterations; i++) {
		update_buffer(input_buffer, BUFFER_SIZE, i);
		sha2_update(&ctx, input_buffer, BUFFER_SIZE);
#ifdef WITH_UST
		tracepoint(benchmark_tracepoint, message, "Hello");
#endif
	}

	if (shared_events_add("end")) {
		exit(EXIT_FAILURE);
	}

	sha2_finish(&ctx, hash);

	exit(EXIT_SUCCESS);
}
