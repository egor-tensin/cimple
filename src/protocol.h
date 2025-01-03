/*
 * Copyright (c) 2023 Egor Tensin <egor@tensin.name>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include "json_rpc.h"
#include "process.h"
#include "run_queue.h"

int request_create_queue_run(struct jsonrpc_request **, const struct run *);
int request_parse_queue_run(const struct jsonrpc_request *, struct run **);

int request_create_new_worker(struct jsonrpc_request **);
int request_parse_new_worker(const struct jsonrpc_request *);

int request_create_start_run(struct jsonrpc_request **, const struct run *);
int request_parse_start_run(const struct jsonrpc_request *, struct run **);

int request_create_finished_run(struct jsonrpc_request **, int run_id,
                                const struct process_output *);
int request_parse_finished_run(const struct jsonrpc_request *, int *run_id,
                               struct process_output **);

int request_create_get_runs(struct jsonrpc_request **);
int request_parse_get_runs(const struct jsonrpc_request *);

int response_create_get_runs(struct jsonrpc_response **, const struct jsonrpc_request *,
                             const struct run_queue *);

#endif
