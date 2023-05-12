/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __CI_QUEUE_H__
#define __CI_QUEUE_H__

#include <sys/queue.h>

struct ci_queue_entry;

int ci_queue_entry_create(struct ci_queue_entry **, const char *url, const char *rev);
void ci_queue_entry_destroy(struct ci_queue_entry *);

const char *ci_queue_entry_get_url(const struct ci_queue_entry *);
const char *ci_queue_entry_get_rev(const struct ci_queue_entry *);

STAILQ_HEAD(ci_queue, ci_queue_entry);

void ci_queue_create(struct ci_queue *);
void ci_queue_destroy(struct ci_queue *);

int ci_queue_is_empty(const struct ci_queue *);

void ci_queue_add_first(struct ci_queue *, struct ci_queue_entry *);
void ci_queue_add_last(struct ci_queue *, struct ci_queue_entry *);

struct ci_queue_entry *ci_queue_remove_first(struct ci_queue *);

#endif
