/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "client.h"
#include "cmd_line.h"
#include "compiler.h"
#include "const.h"
#include "json_rpc.h"
#include "log.h"
#include "net.h"
#include "protocol.h"
#include "run_queue.h"

#include <stdlib.h>
#include <string.h>

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

static int make_request(struct jsonrpc_request **request, int argc, const char **argv)
{
	if (argc < 1) {
		exit_with_usage_err("no action specified");
		return -1;
	}

	if (!strcmp(argv[0], CMD_QUEUE_RUN)) {
		if (argc != 3)
			return -1;

		struct run *run = NULL;
		int ret = run_create(&run, 0, argv[1], argv[2]);
		if (ret < 0)
			return ret;

		ret = request_create_queue_run(request, run);
		run_destroy(run);
		return ret;
	}

	return -1;
}

int client_main(UNUSED const struct client *client, const struct settings *settings, int argc,
                const char **argv)
{
	int ret = 0;

	struct jsonrpc_request *request = NULL;
	ret = make_request(&request, argc, argv);
	if (ret < 0) {
		exit_with_usage_err("invalid request");
		return ret;
	}

	ret = net_connect(settings->host, settings->port);
	if (ret < 0)
		goto free_request;
	int fd = ret;

	ret = jsonrpc_request_send(request, fd);
	if (ret < 0)
		goto close;

	struct jsonrpc_response *response = NULL;
	ret = jsonrpc_response_recv(&response, fd);
	if (ret < 0)
		goto close;

	if (jsonrpc_response_is_error(response)) {
		log_err("server failed to process the request\n");
		ret = -1;
		goto free_response;
	}

free_response:
	jsonrpc_response_destroy(response);

close:
	net_close(fd);

free_request:
	jsonrpc_request_destroy(request);

	return ret;
}
