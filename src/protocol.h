/*
 * Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include "json_rpc.h"
#include "process.h"
#include "run_queue.h"

int run_request_create(struct jsonrpc_request **, const struct run *);
int run_request_parse(const struct jsonrpc_request *, struct run **);

int new_worker_request_create(struct jsonrpc_request **);
int new_worker_request_parse(const struct jsonrpc_request *);

int start_request_create(struct jsonrpc_request **, const struct run *);
int start_request_parse(const struct jsonrpc_request *, struct run **);

int finished_request_create(struct jsonrpc_request **, int run_id, const struct proc_output *);
int finished_request_parse(const struct jsonrpc_request *, int *run_id, struct proc_output **);

#endif
