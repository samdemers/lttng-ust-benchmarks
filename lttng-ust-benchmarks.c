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
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

#include "shared_events.h"

static int print_events(pid_t pid)
{
	struct shared_event *event;
	int i = 0;
	int ret = 0;

	if (shared_events_open(pid) == -1) {
		fprintf(stderr, "Failed to open shared events for pid %d\n",
			pid);
		return -1;
	}

	if (shared_events_rewind() == -1) {
		fprintf(stderr, "Failed to rewind shared events\n");
		shared_events_close();
		return -1;
	}

	printf("{");
	while ((event = shared_events_read())) {
		if (event == (struct shared_event *)-1) {
			fprintf(stderr, "Failed to access shared events\n");
			ret = -1;
			break;
		}

		printf("%c\n\t\"%s\": %lu.%09ld", i ? ',' : ' ',
			event->name,
			(unsigned long) event->ts.tv_sec,
			event->ts.tv_nsec);
		i++;
	}
	printf("\n}\n");
	shared_events_close();
	return ret;
}

static pid_t run_child(char **argv)
{
	pid_t pid = fork(); 
	if (pid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	} else if (pid == 0) {
		/* Child */
		shared_events_close();
		shared_events_clear();
		shared_events_add("exec");

		if (execv(argv[0], &argv[0]) == -1) {
			perror("exec");
		}
		shared_events_close();
		_exit(EXIT_FAILURE);
	}
	/* Parent */
	return pid;
}

static int wait_child(char *name, pid_t pid)
{
	int child_status;

	if (waitpid(pid, &child_status, 0) == -1) {
		perror("waitpid");
		return -1;
	}
	if (WIFEXITED(child_status)) {
		fprintf(stderr,
			"Child \"%s\" [%u] exited with status %d\n",
			name, pid, WEXITSTATUS(child_status));
		return child_status;
	} else {
		fprintf(stderr, "Child \"%s\" [%u] exited abnormally\n",
			name, pid);
		return -1;
	}
}

int main(int argc, char **argv)
{
	if (argc < 2) {

		fprintf(stderr, "Usage: %s <PROGRAM> [<ARGS>]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	fprintf(stderr, "%d CPUs online\n", get_nprocs());
	int nprocs = get_nprocs();

	pid_t *pids = (pid_t *)malloc(sizeof(pid_t) * nprocs);

	for (int i = 0; i < nprocs; i++) {
		pids[i] = run_child(&argv[1]);
	}

	int ret = 0;
	for (int i = 0; i < nprocs; i++) {
		if (wait_child(argv[0], pids[i]))
			ret = -1;
	}

	if (ret != -1) {
		printf("[\n");
		for (int i = 0; i < nprocs; i++) {
			if (print_events(pids[i]) == -1)
				ret = -1;
			if (i < nprocs-1)
				printf(",");
		}
		printf("]\n");
	}

	return ret;
}
