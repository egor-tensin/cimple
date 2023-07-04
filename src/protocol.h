/*
 * Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include "process.h"
#include "run_queue.h"

int msg_run_parse(const struct msg *, struct run **);

int msg_new_worker_create(struct msg **);

int msg_start_create(struct msg **, const struct run *);
int msg_start_parse(const struct msg *, struct run **);

int msg_finished_create(struct msg **, int run_id, const struct proc_output *);
int msg_finished_parse(const struct msg *, int *run_id, struct proc_output *);

#endif
