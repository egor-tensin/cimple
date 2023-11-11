/*
 * Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "protocol.h"
#include "base64.h"
#include "compiler.h"
#include "const.h"
#include "json_rpc.h"
#include "process.h"
#include "run_queue.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

static const char *const run_key_id = "id";
static const char *const run_key_url = "url";
static const char *const run_key_rev = "rev";

int request_create_queue_run(struct jsonrpc_request **request, const struct run *run)
{
	int ret = 0;

	ret = jsonrpc_request_create(request, 1, CMD_QUEUE_RUN, NULL);
	if (ret < 0)
		return ret;
	ret = jsonrpc_request_set_param_string(*request, run_key_url, run_get_url(run));
	if (ret < 0)
		goto free_request;
	ret = jsonrpc_request_set_param_string(*request, run_key_rev, run_get_rev(run));
	if (ret < 0)
		goto free_request;

	return ret;

free_request:
	jsonrpc_request_destroy(*request);

	return ret;
}

int request_parse_queue_run(const struct jsonrpc_request *request, struct run **run)
{
	int ret = 0;

	const char *url = NULL;
	ret = jsonrpc_request_get_param_string(request, run_key_url, &url);
	if (ret < 0)
		return ret;
	const char *rev = NULL;
	ret = jsonrpc_request_get_param_string(request, run_key_rev, &rev);
	if (ret < 0)
		return ret;

	return run_create(run, 0, url, rev);
}

int request_create_new_worker(struct jsonrpc_request **request)
{
	return jsonrpc_notification_create(request, CMD_NEW_WORKER, NULL);
}

int request_parse_new_worker(UNUSED const struct jsonrpc_request *request)
{
	return 0;
}

int request_create_start_run(struct jsonrpc_request **request, const struct run *run)
{
	int ret = 0;

	ret = jsonrpc_notification_create(request, CMD_START_RUN, NULL);
	if (ret < 0)
		return ret;
	ret = jsonrpc_request_set_param_int(*request, run_key_id, run_get_id(run));
	if (ret < 0)
		goto free_request;
	ret = jsonrpc_request_set_param_string(*request, run_key_url, run_get_url(run));
	if (ret < 0)
		goto free_request;
	ret = jsonrpc_request_set_param_string(*request, run_key_rev, run_get_rev(run));
	if (ret < 0)
		goto free_request;

	return ret;

free_request:
	jsonrpc_request_destroy(*request);

	return ret;
}

int request_parse_start_run(const struct jsonrpc_request *request, struct run **run)
{
	int ret = 0;

	int64_t id = 0;
	ret = jsonrpc_request_get_param_int(request, run_key_id, &id);
	if (ret < 0)
		return ret;
	const char *url = NULL;
	ret = jsonrpc_request_get_param_string(request, run_key_url, &url);
	if (ret < 0)
		return ret;
	const char *rev = NULL;
	ret = jsonrpc_request_get_param_string(request, run_key_rev, &rev);
	if (ret < 0)
		return ret;

	return run_create(run, (int)id, url, rev);
}

static const char *const finished_key_run_id = "run_id";
static const char *const finished_key_ec = "exit_code";
static const char *const finished_key_data = "output";

int request_create_finished_run(struct jsonrpc_request **request, int run_id,
                                const struct proc_output *output)
{
	int ret = 0;

	ret = jsonrpc_notification_create(request, CMD_FINISHED_RUN, NULL);
	if (ret < 0)
		return ret;
	ret = jsonrpc_request_set_param_int(*request, finished_key_run_id, run_id);
	if (ret < 0)
		goto free_request;
	ret = jsonrpc_request_set_param_int(*request, finished_key_ec, output->ec);
	if (ret < 0)
		goto free_request;

	char *b64data = NULL;
	ret = base64_encode(output->data, output->data_size, &b64data);
	if (ret < 0)
		goto free_request;

	ret = jsonrpc_request_set_param_string(*request, finished_key_data, b64data);
	free(b64data);
	if (ret < 0)
		goto free_request;

	return ret;

free_request:
	jsonrpc_request_destroy(*request);

	return ret;
}

int request_parse_finished_run(const struct jsonrpc_request *request, int *_run_id,
                               struct proc_output **_output)
{
	int ret = 0;

	struct proc_output *output = NULL;
	ret = proc_output_create(&output);
	if (ret < 0)
		return ret;

	int64_t run_id = 0;
	ret = jsonrpc_request_get_param_int(request, finished_key_run_id, &run_id);
	if (ret < 0)
		goto free_output;

	int64_t ec = -1;
	ret = jsonrpc_request_get_param_int(request, finished_key_ec, &ec);
	if (ret < 0)
		goto free_output;
	output->ec = (int)ec;

	const char *b64data = NULL;
	ret = jsonrpc_request_get_param_string(request, finished_key_data, &b64data);
	if (ret < 0)
		goto free_output;

	ret = base64_decode(b64data, &output->data, &output->data_size);
	if (ret < 0)
		goto free_output;

	*_run_id = (int)run_id;
	*_output = output;
	return ret;

free_output:
	proc_output_destroy(output);

	return ret;
}
