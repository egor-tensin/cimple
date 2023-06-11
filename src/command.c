/*
 * Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "command.h"
#include "log.h"
#include "msg.h"

#include <stdlib.h>
#include <string.h>

struct cmd_dispatcher {
	struct cmd_desc *cmds;
	size_t numof_cmds;
	void *ctx;
};

static int copy_cmd(struct cmd_desc *dest, const struct cmd_desc *src)
{
	dest->name = strdup(src->name);
	if (!dest->name) {
		log_errno("strdup");
		return -1;
	}
	dest->handler = src->handler;
	return 0;
}

static void free_cmd(struct cmd_desc *desc)
{
	free(desc->name);
}

static int copy_cmds(struct cmd_desc *dest, const struct cmd_desc *src, size_t numof_cmds)
{
	size_t numof_copied = 0;
	int ret = 0;

	for (numof_copied = 0; numof_copied < numof_cmds; ++numof_copied) {
		ret = copy_cmd(&dest[numof_copied], &src[numof_copied]);
		if (ret < 0)
			goto free;
	}

	return 0;

free:
	for (size_t i = 0; i < numof_copied; ++i)
		free_cmd(&dest[numof_copied]);

	return -1;
}

static void free_cmds(struct cmd_desc *cmds, size_t numof_cmds)
{
	for (size_t i = 0; i < numof_cmds; ++i)
		free_cmd(&cmds[i]);
}

int cmd_dispatcher_create(struct cmd_dispatcher **_dispatcher, struct cmd_desc *cmds,
                          size_t numof_cmds, void *ctx)
{
	int ret = 0;

	struct cmd_dispatcher *dispatcher = malloc(sizeof(struct cmd_dispatcher));
	if (!dispatcher) {
		log_errno("malloc");
		return -1;
	}

	dispatcher->ctx = ctx;

	dispatcher->cmds = malloc(sizeof(struct cmd_desc) * numof_cmds);
	if (!dispatcher->cmds) {
		log_errno("malloc");
		goto free;
	}
	dispatcher->numof_cmds = numof_cmds;

	ret = copy_cmds(dispatcher->cmds, cmds, numof_cmds);
	if (ret < 0)
		goto free_cmds;

	*_dispatcher = dispatcher;
	return 0;

free_cmds:
	free(dispatcher->cmds);

free:
	free(dispatcher);

	return -1;
}

void cmd_dispatcher_destroy(struct cmd_dispatcher *dispatcher)
{
	free_cmds(dispatcher->cmds, dispatcher->numof_cmds);
	free(dispatcher->cmds);
	free(dispatcher);
}

static int cmd_dispatcher_handle_internal(const struct cmd_dispatcher *dispatcher,
                                          const struct msg *command, struct msg **result, void *arg)
{
	const char *actual_cmd = msg_get_first_string(command);

	for (size_t i = 0; i < dispatcher->numof_cmds; ++i) {
		struct cmd_desc *cmd = &dispatcher->cmds[i];

		if (strcmp(cmd->name, actual_cmd))
			continue;

		return cmd->handler(command, result, arg);
	}

	log_err("Received an unknown command\n");
	msg_dump(command);
	return -1;
}

int cmd_dispatcher_handle(const struct cmd_dispatcher *dispatcher, const struct msg *command,
                          struct msg **result)
{
	return cmd_dispatcher_handle_internal(dispatcher, command, result, dispatcher->ctx);
}

int cmd_dispatcher_handle_conn(int conn_fd, void *_dispatcher)
{
	struct cmd_dispatcher *dispatcher = (struct cmd_dispatcher *)_dispatcher;
	struct msg *request = NULL, *response = NULL;
	int ret = 0;

	struct cmd_conn_ctx *new_ctx = malloc(sizeof(struct cmd_conn_ctx));
	if (!new_ctx) {
		log_errno("malloc");
		return -1;
	}

	new_ctx->fd = conn_fd;
	new_ctx->arg = dispatcher->ctx;

	ret = msg_recv(conn_fd, &request);
	if (ret < 0)
		goto free_ctx;

	ret = cmd_dispatcher_handle_internal(dispatcher, request, &response, new_ctx);
	if (ret < 0)
		goto free_response;

	if (response) {
		ret = msg_send(conn_fd, response);
		if (ret < 0)
			goto free_response;
	}

free_response:
	if (response)
		msg_free(response);

	msg_free(request);

free_ctx:
	free(new_ctx);

	return ret;
}
