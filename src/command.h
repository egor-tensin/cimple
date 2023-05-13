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

typedef int (*command_processor)(int conn_fd, const struct msg *request, void *ctx,
                                 struct msg **response);

struct command_def {
	char *name;
	command_processor processor;
};

struct command_dispatcher;

int command_dispatcher_create(struct command_dispatcher **, struct command_def *, size_t numof_defs,
                              void *ctx);
void command_dispatcher_destroy(struct command_dispatcher *);

int command_dispatcher_msg_handler(const struct command_dispatcher *, int conn_fd,
                                   const struct msg *);

/* This is supposed to be used as an argument to tcp_server_accept. */
int command_dispatcher_conn_handler(int conn_fd, void *dispatcher);

#endif
