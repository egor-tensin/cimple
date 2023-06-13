/*
 * Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __EVENT_LOOP_H__
#define __EVENT_LOOP_H__

struct event_loop;

int event_loop_create(struct event_loop **);
void event_loop_destroy(struct event_loop *);

int event_loop_run(struct event_loop *);

#define EVENT_LOOP_REMOVE 1

typedef int (*event_handler)(struct event_loop *, int fd, short revents, void *arg);

int event_loop_add(struct event_loop *, int fd, short events, event_handler, void *arg);

#endif
