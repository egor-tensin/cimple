/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "worker.h"
#include "ci.h"
#include "command.h"
#include "compiler.h"
#include "const.h"
#include "git.h"
#include "log.h"
#include "msg.h"
#include "process.h"
#include "run_queue.h"
#include "signal.h"

#include <stdlib.h>

struct worker {
	int dummy;
};

int worker_create(struct worker **_worker)
{
	int ret = 0;

	ret = signal_handle_stops();
	if (ret < 0)
		return ret;

	struct worker *worker = malloc(sizeof(struct worker));
	if (!worker) {
		log_errno("malloc");
		return -1;
	}

	ret = libgit_init();
	if (ret < 0)
		goto free;

	*_worker = worker;
	return ret;

free:
	free(worker);

	return ret;
}

void worker_destroy(struct worker *worker)
{
	log("Shutting down\n");

	libgit_shutdown();
	free(worker);
}

static int worker_send_to_server(const struct settings *settings, const struct msg *request,
                                 struct msg **response)
{
	return msg_connect_and_communicate(settings->host, settings->port, request, response);
}

static int worker_send_to_server_argv(const struct settings *settings, const char **argv,
                                      struct msg **response)
{
	struct msg *msg = NULL;
	int ret = 0;

	ret = msg_from_argv(&msg, argv);
	if (ret < 0)
		return ret;

	ret = worker_send_to_server(settings, msg, response);
	msg_free(msg);
	return ret;
}

static int worker_send_new_worker(const struct settings *settings, struct msg **task)
{
	static const char *argv[] = {CMD_NEW_WORKER, NULL};
	return worker_send_to_server_argv(settings, argv, task);
}

static int msg_run_handler(const struct msg *request, struct msg **response, UNUSED void *ctx)
{
	struct run *run = NULL;
	struct proc_output result;
	int ret = 0;

	ret = run_from_msg(&run, request);
	if (ret < 0)
		return ret;

	proc_output_init(&result);

	ret = ci_run_git_repo(run_get_url(run), run_get_rev(run), &result);
	if (ret < 0) {
		log_err("Run failed with an error\n");
		goto free_output;
	}

	proc_output_dump(&result);

	static const char *argv[] = {CMD_COMPLETE, NULL};
	ret = msg_from_argv(response, argv);
	if (ret < 0)
		goto free_output;

free_output:
	proc_output_free(&result);

	run_destroy(run);

	return ret;
}

static struct cmd_desc commands[] = {
    {CMD_RUN, msg_run_handler},
};

static const size_t numof_commands = sizeof(commands) / sizeof(commands[0]);

int worker_main(UNUSED struct worker *worker, const struct settings *settings)
{
	struct msg *task = NULL;
	struct cmd_dispatcher *dispatcher = NULL;
	int ret = 0;

	ret = cmd_dispatcher_create(&dispatcher, commands, numof_commands, NULL);
	if (ret < 0)
		return ret;

	log("Waiting for a new command\n");

	ret = worker_send_new_worker(settings, &task);
	if (ret < 0) {
		if ((errno == EINTR || errno == EINVAL) && global_stop_flag)
			ret = 0;
		goto dispatcher_destroy;
	}

	while (!global_stop_flag) {
		struct msg *result = NULL;

		ret = cmd_dispatcher_handle(dispatcher, task, &result);
		msg_free(task);
		if (ret < 0)
			goto dispatcher_destroy;

		ret = worker_send_to_server(settings, result, &task);
		msg_free(result);
		if (ret < 0) {
			if ((errno == EINTR || errno == EINVAL) && global_stop_flag)
				ret = 0;
			goto dispatcher_destroy;
		}
	}

dispatcher_destroy:
	cmd_dispatcher_destroy(dispatcher);

	return ret;
}
