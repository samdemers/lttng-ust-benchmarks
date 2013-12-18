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

#ifdef WITH_UST
#include <lttng/tracepoint.h>
#include "sum.tp.h"
#endif

#include "shared_events.h"

/*
 * Add side effect to loop to ensure it is not completely optimized away
 * by the compiler. It is _not_ static on purpose, so the compile unit
 * cannot assume it is not touched elsewhere.
 */
int side_effect[2];

static
void print_usage(int argc, char **argv)
{
	fprintf(stderr, "Usage: %s [<iterations>]\n", argv[0]);
}

int main(int argc, char **argv)
{
	long iterations;
	int x = 0, y = 1, i;

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

	if (shared_events_add("start")) {
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < iterations; i++) {
		int z = x + y;

		x = y;
		y = z;
		side_effect[0] = x;
		side_effect[1] = y;
#ifdef WITH_UST
		tracepoint(benchmark_tracepoint, sum, x, y);
#endif
	}

	if (shared_events_add("end")) {
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
