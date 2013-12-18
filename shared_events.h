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

#ifndef SHARED_EVENTS_H
#define SHARED_EVENTS_H

#include <sys/time.h>

#define MAX_EVENT_NAME_LENGTH 256

struct shared_event {
	struct timespec ts;
	char name[MAX_EVENT_NAME_LENGTH];
};

void shared_events_delete(void);
int shared_events_clear(void);
int shared_events_add(const char *name);
struct shared_event *shared_events_read(void);
	
#endif /* SHARED_EVENTS_H */
