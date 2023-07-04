/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __RUN_QUEUE_H__
#define __RUN_QUEUE_H__

#include "msg.h"

#include <sys/queue.h>

struct run;

int run_create(struct run **, int id, const char *url, const char *rev);
void run_destroy(struct run *);

int run_get_id(const struct run *);
const char *run_get_url(const struct run *);
const char *run_get_rev(const struct run *);

void run_set_id(struct run *, int id);

SIMPLEQ_HEAD(run_queue, run);

void run_queue_create(struct run_queue *);
void run_queue_destroy(struct run_queue *);

int run_queue_is_empty(const struct run_queue *);

void run_queue_add_first(struct run_queue *, struct run *);
void run_queue_add_last(struct run_queue *, struct run *);

struct run *run_queue_remove_first(struct run_queue *);

#endif
