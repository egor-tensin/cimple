/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "client.h"
#include "compiler.h"
#include "log.h"
#include "msg.h"
#include "net.h"

#include <stdlib.h>

struct client {
	int dummy;
};

int client_create(struct client **_client)
{
	int ret = 0;

	struct client *client = malloc(sizeof(struct client));
	if (!client) {
		log_errno("malloc");
		return -1;
	}

	*_client = client;
	return ret;
}

void client_destroy(struct client *client)
{
	free(client);
}

int client_main(UNUSED const struct client *client, const struct settings *settings,
                const char **argv)
{
	struct msg *request = NULL, *response = NULL;
	int ret = 0;

	ret = msg_from_argv(&request, argv);
	if (ret < 0)
		return ret;

	ret = msg_connect_and_communicate(settings->host, settings->port, request, &response);
	if (ret < 0)
		goto free_request;

	if (!msg_is_success(response)) {
		log_err("Server failed to process the request\n");
		msg_dump(response);
		ret = -1;
		goto free_response;
	}

free_response:
	msg_free(response);

free_request:
	msg_free(request);

	return ret;
}
