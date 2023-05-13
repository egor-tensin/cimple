/*
 * Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __COMMAND_H__
#define __COMMAND_H__

#include "msg.h"

#include <stdlib.h>

typedef int (*cmd_handler)(int conn_fd, const struct msg *request, void *ctx,
                           struct msg **response);

struct cmd_desc {
	char *name;
	cmd_handler handler;
};

struct cmd_dispatcher;

int cmd_dispatcher_create(struct cmd_dispatcher **, struct cmd_desc *, size_t numof_defs,
                          void *ctx);
void cmd_dispatcher_destroy(struct cmd_dispatcher *);

int cmd_dispatcher_handle_msg(const struct cmd_dispatcher *, int conn_fd, const struct msg *);

/* This is supposed to be used as an argument to tcp_server_accept. */
int cmd_dispatcher_handle_conn(int conn_fd, void *dispatcher);

#endif
