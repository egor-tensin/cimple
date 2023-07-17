/*
 * Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __COMMAND_H__
#define __COMMAND_H__

#include "event_loop.h"
#include "json_rpc.h"

#include <stddef.h>

typedef int (*cmd_handler)(const struct jsonrpc_request *request,
                           struct jsonrpc_response **response, void *ctx);

struct cmd_desc {
	char *name;
	cmd_handler handler;
};

struct cmd_dispatcher;

int cmd_dispatcher_create(struct cmd_dispatcher **, struct cmd_desc *, size_t numof_defs,
                          void *ctx);
void cmd_dispatcher_destroy(struct cmd_dispatcher *);

int cmd_dispatcher_handle(const struct cmd_dispatcher *, const struct jsonrpc_request *command,
                          struct jsonrpc_response **response);

struct cmd_conn_ctx {
	int fd;
	void *arg;
};

/* This is supposed to be used as an argument to tcp_server_accept. */
int cmd_dispatcher_handle_conn(int conn_fd, void *dispatcher);

/* This is supposed to be used as an argument to event_loop_add. */
int cmd_dispatcher_handle_event(struct event_loop *, int fd, short revents, void *arg);

#endif
