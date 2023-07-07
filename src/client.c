/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "client.h"
#include "cmd_line.h"
#include "compiler.h"
#include "log.h"
#include "msg.h"

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

int client_main(UNUSED const struct client *client, const struct settings *settings, int argc,
                const char **argv)
{
	struct msg *response = NULL;
	int ret = 0;

	if (argc < 1) {
		exit_with_usage_err("no message to send to the server");
		return -1;
	}

	ret = msg_connect_and_talk_argv(settings->host, settings->port, argv, &response);
	if (ret < 0 || !response || !msg_is_success(response)) {
		log_err("Failed to connect to server or it couldn't process the request\n");
		if (response)
			msg_dump(response);
		if (!ret)
			ret = -1;
		goto free_response;
	}

free_response:
	if (response)
		msg_free(response);

	return ret;
}
