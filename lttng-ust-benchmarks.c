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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

#include "shared_events.h"

static void print_events() {
	shared_event_t *event;
	printf("{");
	int i = 0;
	while ((event = shared_events_read())) {
		printf("%c\n\t\"%s\": %lu.%06lu", i ? ',' : ' ',
		       event->name, event->tv.tv_sec, event->tv.tv_usec);
		i++;
	}
	printf("\n}\n");
}

int main(int argc, char **argv) {

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <PROGRAM> [<ARGS>]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	atexit(shared_events_delete);
	if (shared_events_clear() == -1) {
		exit(EXIT_FAILURE);
	}
       
	pid_t pid = fork(); 
	if (pid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	} else if (pid == 0) {
		/* Child */
		shared_events_add("exec");

		if (execv(argv[1], &argv[1]) == -1) {
			perror("exec");
		}
		_exit(EXIT_FAILURE);
	} else {
		/* Parent */
		int child_status;
		if (waitpid(pid, &child_status, 0) == -1) {
			perror("waitpid");
			exit(EXIT_FAILURE);
		}
		if (WIFEXITED(child_status)) {
			fprintf(stderr,
				"Child \"%s\" [%u] exited with status %d\n",
				argv[1], pid, WEXITSTATUS(child_status));

			print_events();
			exit(child_status);
		}
		fprintf(stderr, "Child \"%s\" [%u] exited abnormally\n",
			argv[1], pid);
		exit(EXIT_FAILURE);
	}
	return 0;
}
