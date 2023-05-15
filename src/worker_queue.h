/*
 * Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __WORKER_QUEUE_H__
#define __WORKER_QUEUE_H__

#include <sys/queue.h>

struct worker;

int worker_create(struct worker **, int fd);
void worker_destroy(struct worker *);

int worker_get_fd(const struct worker *);

STAILQ_HEAD(worker_queue, worker);

void worker_queue_create(struct worker_queue *);
void worker_queue_destroy(struct worker_queue *);

int worker_queue_is_empty(const struct worker_queue *);

void worker_queue_add_first(struct worker_queue *, struct worker *);
void worker_queue_add_last(struct worker_queue *, struct worker *);

struct worker *worker_queue_remove_first(struct worker_queue *);

#endif
