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

struct command_dispatcher {
	struct command_def *defs;
	size_t numof_defs;
	void *ctx;
};

static int copy_def(struct command_def *dest, const struct command_def *src)
{
	dest->name = strdup(src->name);
	if (!dest->name) {
		log_errno("strdup");
		return -1;
	}
	dest->processor = src->processor;
	return 0;
}

static void free_def(struct command_def *def)
{
	free(def->name);
}

static int copy_defs(struct command_def *dest, const struct command_def *src, size_t numof_defs)
{
	size_t numof_copied, i;
	int ret = 0;

	for (numof_copied = 0; numof_copied < numof_defs; ++numof_copied) {
		ret = copy_def(&dest[numof_copied], &src[numof_copied]);
		if (ret < 0)
			goto free;
	}

	return 0;

free:
	for (i = 0; i < numof_copied; ++i)
		free_def(&dest[numof_copied]);

	return -1;
}

static void free_defs(struct command_def *defs, size_t numof_defs)
{
	size_t i;

	for (i = 0; i < numof_defs; ++i)
		free_def(&defs[i]);
}

int command_dispatcher_create(struct command_dispatcher **_dispatcher, struct command_def *defs,
                              size_t numof_defs, void *ctx)
{
	struct command_dispatcher *dispatcher;
	int ret = 0;

	*_dispatcher = malloc(sizeof(struct command_dispatcher));
	if (!*_dispatcher) {
		log_errno("malloc");
		return -1;
	}
	dispatcher = *_dispatcher;

	dispatcher->defs = malloc(sizeof(struct command_def) * numof_defs);
	if (!dispatcher->defs) {
		log_errno("malloc");
		goto free;
	}
	dispatcher->numof_defs = numof_defs;

	ret = copy_defs(dispatcher->defs, defs, numof_defs);
	if (ret < 0)
		goto free_defs;

	dispatcher->ctx = ctx;
	return 0;

free_defs:
	free(dispatcher->defs);

free:
	free(dispatcher);

	return -1;
}

void command_dispatcher_destroy(struct command_dispatcher *dispatcher)
{
	free_defs(dispatcher->defs, dispatcher->numof_defs);
	free(dispatcher->defs);
	free(dispatcher);
}

int command_dispatcher_msg_handler(const struct command_dispatcher *dispatcher, int conn_fd,
                                   const struct msg *request)
{
	struct msg *response = NULL;
	int ret = 0;

	size_t numof_words = msg_get_length(request);
	if (numof_words == 0)
		goto unknown;

	const char **words = msg_get_words(request);

	for (size_t i = 0; i < dispatcher->numof_defs; ++i) {
		struct command_def *def = &dispatcher->defs[i];

		if (strcmp(def->name, words[0]))
			continue;

		ret = def->processor(conn_fd, request, dispatcher->ctx, &response);
		goto exit;
	}

unknown:
	log_err("Received an unknown command\n");
	ret = -1;
	msg_dump(request);
	goto exit;

exit:
	if (ret < 0 && !response)
		msg_error(&response);
	if (response)
		return msg_send(conn_fd, response);
	return ret;
}

int command_dispatcher_conn_handler(int conn_fd, void *_dispatcher)
{
	struct command_dispatcher *dispatcher = (struct command_dispatcher *)_dispatcher;
	struct msg *request;
	int ret = 0;

	ret = msg_recv(conn_fd, &request);
	if (ret < 0)
		return ret;

	ret = command_dispatcher_msg_handler(dispatcher, conn_fd, request);
	msg_free(request);
	return ret;
}
