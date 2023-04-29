/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "client.h"
#include "log.h"
#include "msg.h"
#include "net.h"

#include <unistd.h>

struct client {
	int fd;
};

int client_create(struct client **_client, const struct settings *settings)
{
	struct client *client;
	int ret = 0;

	*_client = malloc(sizeof(struct client));
	if (!*_client) {
		log_errno("malloc");
		return -1;
	}
	client = *_client;

	ret = net_connect(settings->host, settings->port);
	if (ret < 0)
		goto free;
	client->fd = ret;

	return ret;

free:
	free(client);

	return ret;
}

void client_destroy(struct client *client)
{
	log_errno_if(close(client->fd), "close");
	free(client);
}

int client_main(const struct client *client, int argc, char *argv[])
{
	struct msg request = {argc, argv};
	struct msg response;
	int ret = 0;

	ret = msg_send_and_wait(client->fd, &request, &response);
	if (ret < 0)
		return ret;

	if (msg_is_error(&response)) {
		log_err("Server failed to process the request\n");
		ret = -1;
		goto free_response;
	}

free_response:
	msg_free(&response);

	return ret;
}
