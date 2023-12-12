/*
 * Copyright (c) 2023 Egor Tensin <egor@tensin.name>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "command.h"
#include "compiler.h"
#include "event_loop.h"
#include "json_rpc.h"
#include "log.h"

#include <poll.h>
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
                                          const struct jsonrpc_request *request,
                                          struct jsonrpc_response **result, void *arg)
{
	const char *actual_cmd = jsonrpc_request_get_method(request);

	for (size_t i = 0; i < dispatcher->numof_cmds; ++i) {
		struct cmd_desc *cmd = &dispatcher->cmds[i];

		if (strcmp(cmd->name, actual_cmd))
			continue;

		return cmd->handler(request, result, arg);
	}

	log_err("Received an unknown command: %s\n", actual_cmd);
	return -1;
}

int cmd_dispatcher_handle(const struct cmd_dispatcher *dispatcher,
                          const struct jsonrpc_request *command, struct jsonrpc_response **result)
{
	return cmd_dispatcher_handle_internal(dispatcher, command, result, dispatcher->ctx);
}

static struct cmd_conn_ctx *make_conn_ctx(int fd, void *arg)
{
	struct cmd_conn_ctx *ctx = malloc(sizeof(struct cmd_conn_ctx));
	if (!ctx) {
		log_errno("malloc");
		return NULL;
	}

	ctx->fd = fd;
	ctx->arg = arg;

	return ctx;
}

static int cmd_dispatcher_handle_conn_internal(int conn_fd, struct cmd_dispatcher *dispatcher)
{
	int ret = 0;

	struct cmd_conn_ctx *new_ctx = make_conn_ctx(conn_fd, dispatcher->ctx);
	if (!new_ctx)
		return -1;

	struct jsonrpc_request *request = NULL;
	ret = jsonrpc_request_recv(&request, conn_fd);
	if (ret < 0)
		goto free_ctx;

	const int requires_response = !jsonrpc_request_is_notification(request);

	struct jsonrpc_response *default_response = NULL;
	if (requires_response) {
		ret = jsonrpc_response_create(&default_response, request, NULL);
		if (ret < 0)
			goto free_request;
	}

	struct jsonrpc_response *default_error = NULL;
	if (requires_response) {
		ret = jsonrpc_error_create(&default_error, request, -1, "An error occured");
		if (ret < 0)
			goto free_default_response;
	}

	struct jsonrpc_response *response = NULL;
	ret = cmd_dispatcher_handle_internal(dispatcher, request, &response, new_ctx);

	if (requires_response) {
		struct jsonrpc_response *actual_response = response;
		if (!actual_response) {
			actual_response = ret < 0 ? default_error : default_response;
		}
		if (ret < 0 && !jsonrpc_response_is_error(actual_response)) {
			actual_response = default_error;
		}
		ret = jsonrpc_response_send(actual_response, conn_fd) < 0 ? -1 : ret;
	}

	if (response)
		jsonrpc_response_destroy(response);

	if (default_error)
		jsonrpc_response_destroy(default_error);

free_default_response:
	if (default_response)
		jsonrpc_response_destroy(default_response);

free_request:
	jsonrpc_request_destroy(request);

free_ctx:
	free(new_ctx);

	return ret;
}

int cmd_dispatcher_handle_conn(int conn_fd, void *_dispatcher)
{
	return cmd_dispatcher_handle_conn_internal(conn_fd, (struct cmd_dispatcher *)_dispatcher);
}

int cmd_dispatcher_handle_event(UNUSED struct event_loop *loop, int fd, short revents,
                                void *_dispatcher)
{
	if (!(revents & POLLIN)) {
		log_err("Descriptor %d is not readable\n", fd);
		return -1;
	}

	return cmd_dispatcher_handle_conn_internal(fd, (struct cmd_dispatcher *)_dispatcher);
}
