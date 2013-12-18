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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "shared_events.h"

#define MAX_EVENT_COUNT 256
#define EVENTS_SHM_NAME "/shared_events"

struct shared_events {
	size_t count;
	struct shared_event events[MAX_EVENT_COUNT];
};

static int shm_fd = -1;
static struct shared_events *events = NULL;
static int events_rptr = 0;

static
void shared_events_close(void)
{
	if (events && munmap(events, sizeof(struct shared_events)) == -1)
		perror("munmap");

	if (shm_fd != -1 && close(shm_fd) == -1)
		perror("close");

	shm_fd = -1;
	events = NULL;
	events_rptr = 0;
}

static
int shared_events_init(void)
{
	int shm_created = 0;
	mode_t mode = S_IRUSR | S_IWUSR;

	if (shm_fd != -1)
		return 0;

	shm_fd = shm_open(EVENTS_SHM_NAME, O_RDWR, mode);
	if (errno == ENOENT) {
		shm_created = 1;
		shm_fd = shm_open(EVENTS_SHM_NAME, O_RDWR | O_CREAT, mode);
	}
	if (shm_fd == -1) {
		perror("shm_open");
		return -1;
	}

	if (ftruncate(shm_fd, sizeof(struct shared_events)) == -1) {
		perror("ftruncate");
		shared_events_close();
		if (shm_created)
			shared_events_delete();
		return -1;
	}
	
	events = mmap(NULL, sizeof(struct shared_events),
			PROT_READ | PROT_WRITE, MAP_SHARED,
			shm_fd, 0);
	if (events == MAP_FAILED) {
		perror("mmap");
		shared_events_close();
		if (shm_created)
			shared_events_delete();
		return -1;
	}

	atexit(shared_events_close);
	return 0;
}

void shared_events_delete(void)
{
	shared_events_close();
	if (shm_unlink(EVENTS_SHM_NAME) == -1)
		perror("shm_unlink");
}

int shared_events_clear(void)
{
	if (shared_events_init() == -1)
		return -1;
	assert(events);
	events->count = 0;
	return 0;
}

int shared_events_add(const char *name)
{
	struct timespec *ts, tmpts;
	struct shared_event *event;
	int ret;

	/*
	 * Use clock_gettime with CLOCK_MONOTONIC, because
	 * CLOCK_REALTIME and gettimeofday() suffer from major NTP
	 * corrections which can skew results.
	 */
	ret = clock_gettime(CLOCK_MONOTONIC, &tmpts);
	if (ret) {
		perror("clock_gettime");
		return -1;
	}

	if (shared_events_init() == -1)
		return -1;

	assert(events);
	if (events->count == MAX_EVENT_COUNT) {
		fprintf(stderr, "Event buffer is full\n");
		return -1;
	}

	event = &events->events[events->count];
	strncpy(event->name, name, MAX_EVENT_NAME_LENGTH);
	events->count++;

	if (!strcmp(name, "start")) {
		/*
		 * For the "start" event, take timestamp at the end of
		 * this function.
		 */
		ret = clock_gettime(CLOCK_MONOTONIC, &event->ts);
		if (ret) {
			perror("clock_gettime");
			return -1;
		}
	} else {
		/*
		 * For all other events, take timestamp at the start of
		 * this function. This mainly includes the "end" event.
		 */
		event->ts = tmpts;
	}
	return 0;
}

struct shared_event *shared_events_read(void)
{
	if (shared_events_init() == -1)
		return (struct shared_event *) -1;
	assert(events);
	if (events_rptr == events->count)
		return NULL;
	return &events->events[events_rptr++];
}
