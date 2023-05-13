/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __RUN_QUEUE_H__
#define __RUN_QUEUE_H__

#include <sys/queue.h>

struct run_queue_entry;

int run_queue_entry_create(struct run_queue_entry **, const char *url, const char *rev);
void run_queue_entry_destroy(struct run_queue_entry *);

const char *run_queue_entry_get_url(const struct run_queue_entry *);
const char *run_queue_entry_get_rev(const struct run_queue_entry *);

STAILQ_HEAD(run_queue, run_queue_entry);

void run_queue_create(struct run_queue *);
void run_queue_destroy(struct run_queue *);

int run_queue_is_empty(const struct run_queue *);

void run_queue_add_first(struct run_queue *, struct run_queue_entry *);
void run_queue_add_last(struct run_queue *, struct run_queue_entry *);

struct run_queue_entry *run_queue_remove_first(struct run_queue *);

#endif
