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
#endif

#include "sha2.h"
#include "shared_events.h"

#define BUFFER_SIZE (1<<20)
unsigned char buffer[BUFFER_SIZE];

int main(int argc, char **argv) {
	if (shared_events_add("main") == -1)
		exit(EXIT_FAILURE);

	sha2_context ctx;
	sha2_starts(&ctx, 0);

	shared_events_add("start");
	sha2_update(&ctx, buffer, BUFFER_SIZE);
	shared_events_add("end");

	unsigned char hash[32];
	sha2_finish(&ctx, hash);
}
