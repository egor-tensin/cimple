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
#include "event_loop.h"
#include "git.h"
#include "log.h"
#include "msg.h"
#include "net.h"
#include "process.h"
#include "protocol.h"
#include "run_queue.h"
#include "signal.h"

#include <poll.h>
#include <stdlib.h>
#include <string.h>

struct worker {
	struct settings *settings;

	int stopping;

	struct cmd_dispatcher *cmd_dispatcher;

	struct event_loop *event_loop;
	int signalfd;
};

static struct settings *worker_settings_copy(const struct settings *src)
{
	struct settings *result = malloc(sizeof(*src));
	if (!result) {
		log_errno("malloc");
		return NULL;
	}

	result->host = strdup(src->host);
	if (!result->host) {
		log_errno("strdup");
		goto free_result;
	}

	result->port = strdup(src->port);
	if (!result->port) {
		log_errno("strdup");
		goto free_host;
	}

	return result;

free_host:
	free((void *)result->host);

free_result:
	free(result);

	return NULL;
}

static void worker_settings_destroy(struct settings *settings)
{
	free((void *)settings->port);
	free((void *)settings->host);
	free(settings);
}

static int worker_set_stopping(UNUSED struct event_loop *loop, UNUSED int fd, UNUSED short revents,
                               void *_worker)
{
	struct worker *worker = (struct worker *)_worker;
	worker->stopping = 1;
	return 0;
}

static int worker_handle_cmd_start(const struct msg *request, UNUSED struct msg **response,
                                   void *_ctx)
{
	struct cmd_conn_ctx *ctx = (struct cmd_conn_ctx *)_ctx;
	struct worker *worker = (struct worker *)ctx->arg;
	int ret = 0;

	struct run *run = NULL;

	ret = msg_start_parse(request, &run);
	if (ret < 0)
		return ret;

	struct proc_output result;
	proc_output_init(&result);

	ret = ci_run_git_repo(run_get_url(run), run_get_rev(run), &result);
	if (ret < 0) {
		log_err("Run failed with an error\n");
		goto free_output;
	}

	proc_output_dump(&result);

	struct msg *finished_msg = NULL;

	ret = msg_finished_create(&finished_msg, run_get_id(run), &result);
	if (ret < 0)
		goto free_output;

	ret = msg_connect_and_talk(worker->settings->host, worker->settings->port, finished_msg,
	                           NULL);
	if (ret < 0)
		goto free_finished_msg;

free_finished_msg:
	msg_free(finished_msg);

free_output:
	proc_output_free(&result);

	run_destroy(run);

	return ret;
}

static struct cmd_desc commands[] = {
    {CMD_START, worker_handle_cmd_start},
};

static const size_t numof_commands = sizeof(commands) / sizeof(commands[0]);

int worker_create(struct worker **_worker, const struct settings *settings)
{
	int ret = 0;

	struct worker *worker = malloc(sizeof(struct worker));
	if (!worker) {
		log_errno("malloc");
		return -1;
	}

	worker->settings = worker_settings_copy(settings);
	if (!worker->settings) {
		ret = -1;
		goto free;
	}

	worker->stopping = 0;

	ret = cmd_dispatcher_create(&worker->cmd_dispatcher, commands, numof_commands, worker);
	if (ret < 0)
		goto free_settings;

	ret = event_loop_create(&worker->event_loop);
	if (ret < 0)
		goto destroy_cmd_dispatcher;

	ret = signalfd_create_sigterms();
	if (ret < 0)
		goto destroy_event_loop;
	worker->signalfd = ret;

	ret = event_loop_add(worker->event_loop, worker->signalfd, POLLIN, worker_set_stopping,
	                     worker);
	if (ret < 0)
		goto close_signalfd;

	ret = libgit_init();
	if (ret < 0)
		goto close_signalfd;

	*_worker = worker;
	return ret;

close_signalfd:
	signalfd_destroy(worker->signalfd);

destroy_event_loop:
	event_loop_destroy(worker->event_loop);

destroy_cmd_dispatcher:
	cmd_dispatcher_destroy(worker->cmd_dispatcher);

free_settings:
	worker_settings_destroy(worker->settings);

free:
	free(worker);

	return ret;
}

void worker_destroy(struct worker *worker)
{
	log("Shutting down\n");

	libgit_shutdown();
	signalfd_destroy(worker->signalfd);
	event_loop_destroy(worker->event_loop);
	cmd_dispatcher_destroy(worker->cmd_dispatcher);
	worker_settings_destroy(worker->settings);
	free(worker);
}

int worker_main(struct worker *worker)
{
	int ret = 0;

	while (!worker->stopping) {
		ret = net_connect(worker->settings->host, worker->settings->port);
		if (ret < 0)
			return ret;

		const int fd = ret;
		struct msg *new_worker_msg = NULL;

		ret = msg_new_worker_create(&new_worker_msg);
		if (ret < 0)
			return ret;

		ret = msg_send(fd, new_worker_msg);
		msg_free(new_worker_msg);
		if (ret < 0)
			return ret;

		ret = event_loop_add_once(worker->event_loop, fd, POLLIN,
		                          cmd_dispatcher_handle_event, worker->cmd_dispatcher);
		if (ret < 0)
			return ret;

		log("Waiting for a new command\n");

		ret = event_loop_run(worker->event_loop);
		if (ret < 0)
			return ret;
	}

	return ret;
}
