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
#include "net.h"
#include "process.h"
#include "signal.h"

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

struct worker {
	int fd;

	/* TODO: these are not used, but they should be! */
	pthread_mutex_t task_mtx;
	pthread_t task;
	int task_active;
};

int worker_create(struct worker **_worker, const struct settings *settings)
{
	int ret = 0;

	struct worker *worker = malloc(sizeof(struct worker));
	if (!worker) {
		log_errno("malloc");
		return -1;
	}

	ret = libgit_init();
	if (ret < 0)
		goto free;

	ret = net_connect(settings->host, settings->port);
	if (ret < 0)
		goto git_shutdown;
	worker->fd = ret;

	*_worker = worker;
	return ret;

git_shutdown:
	libgit_shutdown();

free:
	free(worker);

	return ret;
}

void worker_destroy(struct worker *worker)
{
	log("Shutting down\n");

	log_errno_if(close(worker->fd), "close");
	libgit_shutdown();
	free(worker);
}

static int msg_send_new_worker(const struct worker *worker)
{
	static const char *argv[] = {CMD_NEW_WORKER, NULL};
	struct msg *msg = NULL;
	int ret = 0;

	ret = msg_from_argv(&msg, argv);
	if (ret < 0)
		return ret;

	ret = msg_send(worker->fd, msg);
	msg_free(msg);
	return ret;
}

static int msg_ci_run_do(const char *url, const char *rev, struct proc_output *result)
{
	int ret = 0;

	ret = ci_run_git_repo(url, rev, result);
	if (ret < 0) {
		log_err("Run failed with an error\n");
		return ret;
	}

	proc_output_dump(result);
	return 0;
}

static int msg_ci_run_handler(UNUSED int conn_fd, const struct msg *request, UNUSED void *_worker,
                              struct msg **response)
{
	struct proc_output result;
	int ret = 0;

	if (msg_get_length(request) != 3) {
		log_err("Invalid number of arguments for a message\n");
		msg_dump(request);
		return -1;
	}

	const char **words = msg_get_words(request);

	proc_output_init(&result);
	ret = msg_ci_run_do(words[1], words[2], &result);
	proc_output_free(&result);

	if (ret < 0)
		return ret;

	return msg_success(response);
}

static struct cmd_desc cmds[] = {
    {CMD_RUN, msg_ci_run_handler},
};

int worker_main(struct worker *worker, UNUSED int argc, UNUSED char *argv[])
{
	struct cmd_dispatcher *dispatcher = NULL;
	int ret = 0;

	ret = signal_install_global_handler();
	if (ret < 0)
		return ret;

	ret = msg_send_new_worker(worker);
	if (ret < 0)
		return ret;

	ret = cmd_dispatcher_create(&dispatcher, cmds, sizeof(cmds) / sizeof(cmds[0]), worker);
	if (ret < 0)
		return ret;

	while (!global_stop_flag) {
		struct msg *request = NULL;

		log("Waiting for a new command\n");

		ret = msg_recv(worker->fd, &request);
		if (ret < 0) {
			if (errno == EINVAL && global_stop_flag)
				ret = 0;
			goto dispatcher_destroy;
		}

		ret = cmd_dispatcher_handle_msg(dispatcher, worker->fd, request);
		msg_free(request);
		if (ret < 0)
			goto dispatcher_destroy;
	}

dispatcher_destroy:
	cmd_dispatcher_destroy(dispatcher);

	return ret;
}
