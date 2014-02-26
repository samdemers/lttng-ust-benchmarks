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
#define MAX_SHM_NAME_LEN 32
#define SHM_BASE_NAME "/shared_events"

struct shared_events {
	size_t count;
	struct shared_event events[MAX_EVENT_COUNT];
};

struct shared_events_ctx {
	char name[MAX_SHM_NAME_LEN];
	int fd;
	struct shared_events *events;
	int rptr;
};

static struct shared_events_ctx *ctx = NULL;

static void shm_name(char *buf, size_t max_len, pid_t pid)
{
	snprintf(buf, max_len, "%s_%d", SHM_BASE_NAME, pid);
}

void shared_events_close()
{
	if (!ctx)
		return;

	if (munmap(ctx->events, sizeof(struct shared_events)) == -1)
		perror("munmap");

	if (close(ctx->fd) == -1)
		perror("close");

	free(ctx);
	ctx = NULL;
}

int shared_events_open(pid_t pid)
{
	if (ctx)
		return 0;

	int shm_created = 0;
	mode_t mode = S_IRUSR | S_IWUSR;

	ctx = (struct shared_events_ctx *)
		malloc(sizeof(struct shared_events_ctx));
	if (!ctx) {
		perror("malloc");
		return -1;
	}
	shm_name(ctx->name, MAX_SHM_NAME_LEN, pid);

	ctx->fd = shm_open(ctx->name, O_RDWR, mode);
	if (errno == ENOENT) {
		shm_created = 1;
		ctx->fd = shm_open(ctx->name, O_RDWR | O_CREAT, mode);
	}
	if (ctx->fd == -1) {
		perror("shm_open");
		return -1;
	}

	if (ftruncate(ctx->fd, sizeof(struct shared_events)) == -1) {
		perror("ftruncate");
		shared_events_close();
		if (shm_created)
			shared_events_delete(pid);
		return -1;
	}
	
	ctx->events = mmap(NULL, sizeof(struct shared_events),
				PROT_READ | PROT_WRITE, MAP_SHARED,
				ctx->fd, 0);
	if (ctx->events == MAP_FAILED) {
		perror("mmap");
		shared_events_close();
		if (shm_created)
			shared_events_delete(pid);
		return -1;
	}

	return 0;
}

void shared_events_delete(pid_t pid)
{
	char name[MAX_SHM_NAME_LEN];
	shm_name(ctx->name, MAX_SHM_NAME_LEN, pid);
	if (shm_unlink(name) == -1)
		perror("shm_unlink");
}

int shared_events_clear(void)
{
	if (!ctx && shared_events_open(getpid()) == -1)
		return -1;
	assert(ctx->events);
	ctx->events->count = 0;
	return 0;
}

int shared_events_add(const char *name)
{
	struct timespec tmpts;
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

	if (!ctx && shared_events_open(getpid()) == -1)
		return -1;

	assert(events);
	if (ctx->events->count == MAX_EVENT_COUNT) {
		fprintf(stderr, "Event buffer is full\n");
		return -1;
	}

	event = &ctx->events->events[ctx->events->count];
	strncpy(event->name, name, MAX_EVENT_NAME_LENGTH);
	ctx->events->count++;

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
	if (!ctx && shared_events_open(getpid()) == -1)
		return (struct shared_event *)-1;
	assert(ctx->events);
	if (ctx->rptr == ctx->events->count)
		return NULL;
	return &ctx->events->events[ctx->rptr++];
}

int shared_events_rewind(void)
{
	if (!ctx && shared_events_open(getpid()) == -1)
		return -1;
	assert(ctx->events);
	ctx->rptr = 0;
	return 0;
}
