#include "worker.h"
#include "ci.h"
#include "const.h"
#include "git.h"
#include "log.h"
#include "msg.h"
#include "net.h"
#include "process.h"
#include "signal.h"

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int worker_create(struct worker *worker, const struct settings *settings)
{
	int ret = 0;

	ret = libgit_init();
	if (ret < 0)
		return ret;

	ret = net_connect(settings->host, settings->port);
	if (ret < 0)
		goto git_shutdown;
	worker->fd = ret;

	return ret;

git_shutdown:
	libgit_shutdown();

	return ret;
}

void worker_destroy(struct worker *worker)
{
	print_log("Shutting down\n");

	check_errno(close(worker->fd), "close");
	libgit_shutdown();
}

static int msg_send_worker_new(const struct worker *worker)
{
	static char *argv[] = {CMD_WORKER_NEW, NULL};
	struct msg msg;
	int ret = 0;

	ret = msg_from_argv(&msg, argv);
	if (ret < 0)
		return ret;

	ret = msg_send(worker->fd, &msg);
	if (ret < 0)
		goto free_msg;

free_msg:
	msg_free(&msg);

	return ret;
}

static int msg_ci_run_do(const char *url, const char *rev, struct proc_output *result)
{
	int ret = 0;

	ret = ci_run_git_repo(url, rev, result);
	if (ret < 0) {
		print_error("Run failed with an error\n");
		return ret;
	}

	print_log("Process exit code: %d\n", result->ec);
	print_log("Process output:\n%s", result->output);
	if (!result->output || !result->output_len ||
	    result->output[result->output_len - 1] != '\n')
		print_log("\n");

	return 0;
}

static int msg_ci_run_handler(struct worker *worker, const struct msg *request)
{
	struct msg response;
	struct proc_output result;
	int ret = 0;

	proc_output_init(&result);
	ret = msg_ci_run_do(request->argv[1], request->argv[2], &result);
	proc_output_free(&result);

	if (ret < 0)
		ret = msg_error(&response);
	else
		ret = msg_success(&response);

	if (ret < 0)
		return ret;

	ret = msg_send(worker->fd, &response);
	msg_free(&response);
	return ret;
}

static int msg_ci_run_parser(const struct msg *msg)
{
	if (msg->argc != 3) {
		print_error("Invalid number of arguments for a message: %d\n", msg->argc);
		return 0;
	}

	return 1;
}

typedef int (*msg_parser)(const struct msg *msg);
typedef int (*msg_handler)(struct worker *, const struct msg *);

struct msg_descr {
	const char *cmd;
	msg_parser parser;
	msg_handler handler;
};

struct msg_descr messages[] = {
    {CMD_CI_RUN, msg_ci_run_parser, msg_ci_run_handler},
};

static int worker_msg_handler(struct worker *worker, const struct msg *request)
{
	if (request->argc == 0)
		goto unknown_request;

	size_t numof_messages = sizeof(messages) / sizeof(messages[0]);

	for (size_t i = 0; i < numof_messages; ++i) {
		if (strcmp(messages[i].cmd, request->argv[0]))
			continue;
		if (!messages[i].parser(request))
			continue;
		return messages[i].handler(worker, request);
	}

unknown_request:
	print_error("Received an unknown message\n");
	msg_dump(request);
	struct msg response;
	msg_error(&response);
	return msg_send(worker->fd, &response);
}

int worker_main(struct worker *worker, UNUSED int argc, UNUSED char *argv[])
{
	int ret = 0;

	ret = msg_send_worker_new(worker);
	if (ret < 0)
		return ret;

	while (!global_stop_flag) {
		struct msg request;

		print_log("Waiting for a new command\n");

		ret = msg_recv(worker->fd, &request);
		if (ret < 0)
			return ret;

		ret = worker_msg_handler(worker, &request);
		msg_free(&request);
		if (ret < 0)
			return ret;
	}

	return ret;
}
