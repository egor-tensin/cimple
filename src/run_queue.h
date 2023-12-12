/*
 * Copyright (c) 2022 Egor Tensin <egor@tensin.name>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __RUN_QUEUE_H__
#define __RUN_QUEUE_H__

#include <json-c/json_object.h>

#include <sys/queue.h>

enum run_status {
	RUN_STATUS_CREATED = 1,
	RUN_STATUS_FINISHED = 2,
};

struct run;

int run_new(struct run **, int id, const char *repo_url, const char *repo_rev, enum run_status,
            int exit_code);
void run_destroy(struct run *);

int run_queued(struct run **, const char *repo_url, const char *repo_rev);
int run_created(struct run **, int id, const char *repo_url, const char *repo_rev);

int run_to_json(const struct run *, struct json_object **);

int run_get_id(const struct run *);
const char *run_get_repo_url(const struct run *);
const char *run_get_repo_rev(const struct run *);

void run_set_id(struct run *, int id);

SIMPLEQ_HEAD(run_queue, run);

void run_queue_create(struct run_queue *);
void run_queue_destroy(struct run_queue *);

int run_queue_to_json(const struct run_queue *, struct json_object **);

int run_queue_is_empty(const struct run_queue *);

void run_queue_add_first(struct run_queue *, struct run *);
void run_queue_add_last(struct run_queue *, struct run *);

struct run *run_queue_remove_first(struct run_queue *);

#endif
