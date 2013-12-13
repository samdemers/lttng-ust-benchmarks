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

int main(int argc, char **argv) {
	if (shared_events_add("main") == -1)
		exit(EXIT_FAILURE);

	if (argc < 2)
		exit(EXIT_SUCCESS);

	long iterations = strtol(argv[1], (char **)NULL, 10);
	if (iterations <= 0) {
		fprintf(stderr, "Usage: %s [<iterations>]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	shared_events_add("start");

	int x = 0;
	int y = 1;
	for (int i = 0; i < iterations; i++) {
		int z = x + y;
		x = y;
		y = z;
#ifdef WITH_UST
		tracepoint(benchmark_tracepoint, sum, x, y);
#endif
	}

	shared_events_add("end");
}
