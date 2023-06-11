/*
 * Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __EVENT_LOOP_H__
#define __EVENT_LOOP_H__

#include <poll.h>
#include <sys/queue.h>

struct event_loop;

int event_loop_create(struct event_loop **);
void event_loop_destroy(struct event_loop *);

int event_loop_run(struct event_loop *);

#define EVENT_LOOP_REMOVE 1

typedef int (*event_loop_handler)(struct event_loop *, int fd, short revents, void *arg);

struct event_fd {
	int fd;
	short events;
	event_loop_handler handler;
	void *arg;

	SIMPLEQ_ENTRY(event_fd) entries;
};

int event_loop_add(struct event_loop *, const struct event_fd *);

#endif
