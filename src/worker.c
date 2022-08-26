#include "worker.h"
#include "ci.h"
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

	ret = pthread_mutex_init(&worker->task_mtx, NULL);
	if (ret < 0) {
		print_errno("pthread_mutex_init");
		goto close;
	}

	worker->task_active = 0;
	return ret;

close:
	close(worker->fd);

git_shutdown:
	libgit_shutdown();

	return ret;
}

void worker_destroy(struct worker *worker)
{
	print_log("Shutting down\n");

	if (worker->task_active) {
		if (pthread_join(worker->task, NULL))
			print_errno("pthread_join");
		worker->task_active = 0;
	}
	if (pthread_mutex_destroy(&worker->task_mtx))
		print_errno("pthread_mutex_destroy");
	close(worker->fd);
	libgit_shutdown();
}

static int msg_send_new_worker(const struct worker *worker)
{
	static char *argv[] = {"new_worker", NULL};
	struct msg msg;
	int response, ret = 0;

	ret = msg_from_argv(&msg, argv);
	if (ret < 0)
		return ret;

	ret = msg_send_and_wait(worker->fd, &msg, &response);
	if (ret < 0)
		goto free_msg;

	if (response < 0) {
		print_error("Failed to register\n");
		ret = response;
		goto free_msg;
	}

free_msg:
	msg_free(&msg);

	return ret;
}

typedef int (*worker_task_body)(const struct msg *);

static int msg_body_ci_run(const struct msg *msg)
{
	const char *url = msg->argv[1];
	const char *rev = msg->argv[2];
	struct proc_output result;
	int ret = 0;

	ret = ci_run_git_repo(url, rev, &result);
	if (ret < 0) {
		print_error("Run failed with an error\n");
		return ret;
	}

	print_log("Process exit code: %d\n", result.ec);
	print_log("Process output:\n%s", result.output);
	if (!result.output || !result.output_len || result.output[result.output_len - 1] != '\n')
		print_log("\n");
	free(result.output);

	return ret;
}

typedef worker_task_body (*worker_msg_parser)(const struct msg *);

static worker_task_body parse_msg_ci_run(const struct msg *msg)
{
	if (msg->argc != 3) {
		print_error("Invalid number of arguments for a message: %d\n", msg->argc);
		return NULL;
	}

	return msg_body_ci_run;
}

struct worker_msg_parser_it {
	const char *msg;
	worker_msg_parser parser;
};

struct worker_msg_parser_it worker_msg_parsers[] = {
    {"ci_run", parse_msg_ci_run},
};

struct worker_task_context {
	struct msg *msg;
	worker_task_body body;
};

static void *worker_task_wrapper(void *_ctx)
{
	struct worker_task_context *ctx = (struct worker_task_context *)_ctx;

	ctx->body(ctx->msg);
	msg_free(ctx->msg);
	free(ctx->msg);
	free(ctx);
	return NULL;
}

static int worker_schedule_task(struct worker *worker, const struct msg *msg, worker_task_body body)
{
	struct worker_task_context *ctx;
	pthread_attr_t attr;
	int ret = 0;

	ctx = malloc(sizeof(*ctx));
	if (!ctx) {
		print_errno("malloc");
		return -1;
	}
	ctx->body = body;

	ctx->msg = msg_copy(msg);
	if (!ctx->msg) {
		ret = -1;
		goto free_ctx;
	}

	ret = pthread_attr_init(&attr);
	if (ret < 0) {
		print_errno("pthread_attr_init");
		goto free_msg;
	}

	ret = signal_set_thread_attr(&attr);
	if (ret < 0)
		goto free_attr;

	ret = pthread_create(&worker->task, NULL, worker_task_wrapper, ctx);
	if (ret < 0) {
		print_errno("pthread_create");
		goto free_attr;
	}
	worker->task_active = 1;
	pthread_attr_destroy(&attr);

	return ret;

free_attr:
	pthread_attr_destroy(&attr);

free_msg:
	msg_free(ctx->msg);
	free(ctx->msg);

free_ctx:
	free(ctx);

	return ret;
}

static int worker_msg_handler(struct worker *worker, const struct msg *msg)
{
	if (worker->task_active) {
		int ret = pthread_tryjoin_np(worker->task, NULL);
		switch (ret) {
		case 0:
			worker->task_active = 0;
			break;
		case EBUSY:
			break;
		default:
			print_errno("pthread_tryjoin_np");
			return ret;
		}
	}

	if (worker->task_active) {
		print_error("Worker is busy\n");
		return -1;
	}

	if (msg->argc == 0) {
		print_error("Received an empty message\n");
		return -1;
	}

	size_t numof_parsers = sizeof(worker_msg_parsers) / sizeof(worker_msg_parsers[0]);

	for (size_t i = 0; i < numof_parsers; ++i) {
		const struct worker_msg_parser_it *it = &worker_msg_parsers[i];
		if (strcmp(it->msg, msg->argv[0]))
			continue;

		worker_task_body body = it->parser(msg);
		if (!body)
			return -1;

		return worker_schedule_task(worker, msg, body);
	}

	return msg_dump_unknown(msg);
}

static int worker_msg_handler_lock(const struct msg *msg, void *_worker)
{
	struct worker *worker = (struct worker *)_worker;
	int ret = 0;

	ret = pthread_mutex_lock(&worker->task_mtx);
	if (ret < 0) {
		print_errno("pthread_mutex_lock");
		return -1;
	}

	ret = worker_msg_handler(worker, msg);

	if (pthread_mutex_unlock(&worker->task_mtx))
		print_errno("pthread_mutex_unlock");

	return ret;
}

int worker_main(struct worker *worker, int, char *[])
{
	int ret = 0;

	ret = msg_send_new_worker(worker);
	if (ret < 0)
		return ret;

	while (!global_stop_flag) {
		print_log("Waiting for a new command\n");

		ret = msg_recv_and_handle(worker->fd, worker_msg_handler_lock, worker);
		if (ret < 0)
			return ret;
	}

	return ret;
}
