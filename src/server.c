#include "server.h"
#include "ci_queue.h"
#include "log.h"
#include "msg.h"
#include "signal.h"
#include "tcp_server.h"
#include "worker_queue.h"

#include <pthread.h>
#include <string.h>
#include <unistd.h>

static int server_has_runs_and_workers(const struct server *server)
{
	return !ci_queue_is_empty(&server->ci_queue) &&
	       !worker_queue_is_empty(&server->worker_queue);
}

static int server_scheduler_iteration(struct server *server)
{
	struct worker_queue_entry *worker;
	struct ci_queue_entry *ci_run;
	struct msg msg;
	int response, ret = 0;

	worker = worker_queue_pop(&server->worker_queue);
	ci_run = ci_queue_pop(&server->ci_queue);

	char *argv[] = {"ci_run", ci_run->url, ci_run->rev, NULL};

	ret = msg_from_argv(&msg, argv);
	if (ret < 0)
		goto requeue_ci_run;

	ret = msg_send_and_wait(worker->fd, &msg, &response);
	if (ret < 0)
		goto free_msg;

	if (response < 0) {
		print_error("Failed to schedule a CI run\n");
	}

	msg_free(&msg);

	ci_queue_entry_destroy(ci_run);

	/* FIXME: Don't mark worker as free! */
	worker_queue_push_head(&server->worker_queue, worker);

	return 0;

free_msg:
	msg_free(&msg);

requeue_ci_run:
	ci_queue_push_head(&server->ci_queue, ci_run);

	worker_queue_push_head(&server->worker_queue, worker);

	return ret;
}

static void *server_scheduler(void *_server)
{
	struct server *server = (struct server *)_server;
	int ret = 0;

	ret = pthread_mutex_lock(&server->server_mtx);
	if (ret) {
		pthread_print_errno(ret, "pthread_mutex_lock");
		goto exit;
	}

	while (1) {
		while (!server->stopping && !server_has_runs_and_workers(server)) {
			ret = pthread_cond_wait(&server->server_cv, &server->server_mtx);
			if (ret) {
				pthread_print_errno(ret, "pthread_cond_wait");
				goto unlock;
			}
		}

		if (server->stopping)
			goto unlock;

		ret = server_scheduler_iteration(server);
		if (ret < 0)
			goto unlock;
	}

unlock:
	pthread_check(pthread_mutex_unlock(&server->server_mtx), "pthread_mutex_unlock");

exit:
	return NULL;
}

int server_create(struct server *server, const struct settings *settings)
{
	pthread_attr_t scheduler_attr;
	int ret = 0;

	ret = pthread_mutex_init(&server->server_mtx, NULL);
	if (ret) {
		pthread_print_errno(ret, "pthread_mutex_init");
		goto fail;
	}

	ret = pthread_cond_init(&server->server_cv, NULL);
	if (ret) {
		pthread_print_errno(ret, "pthread_cond_init");
		goto destroy_mtx;
	}

	server->stopping = 0;

	ret = tcp_server_create(&server->tcp_server, settings->port);
	if (ret < 0)
		goto destroy_cv;

	worker_queue_create(&server->worker_queue);

	ci_queue_create(&server->ci_queue);

	ret = pthread_attr_init(&scheduler_attr);
	if (ret) {
		pthread_print_errno(ret, "pthread_attr_init");
		goto destroy_ci_queue;
	}

	ret = signal_set_thread_attr(&scheduler_attr);
	if (ret)
		goto destroy_attr;

	ret = pthread_create(&server->scheduler, &scheduler_attr, server_scheduler, server);
	if (ret) {
		pthread_print_errno(ret, "pthread_create");
		goto destroy_attr;
	}

	pthread_check(pthread_attr_destroy(&scheduler_attr), "pthread_attr_destroy");

	return ret;

destroy_attr:
	pthread_check(pthread_attr_destroy(&scheduler_attr), "pthread_attr_destroy");

destroy_ci_queue:
	ci_queue_destroy(&server->ci_queue);

	worker_queue_destroy(&server->worker_queue);

	tcp_server_destroy(&server->tcp_server);

destroy_cv:
	pthread_check(pthread_cond_destroy(&server->server_cv), "pthread_cond_destroy");

destroy_mtx:
	pthread_check(pthread_mutex_destroy(&server->server_mtx), "pthread_mutex_destroy");

fail:
	return ret;
}

void server_destroy(struct server *server)
{
	print_log("Shutting down\n");

	pthread_check(pthread_join(server->scheduler, NULL), "pthread_join");
	ci_queue_destroy(&server->ci_queue);
	worker_queue_destroy(&server->worker_queue);
	tcp_server_destroy(&server->tcp_server);
	pthread_check(pthread_cond_destroy(&server->server_cv), "pthread_cond_destroy");
	pthread_check(pthread_mutex_destroy(&server->server_mtx), "pthread_mutex_destroy");
}

struct msg_context {
	struct server *server;
	int client_fd;
};

static int msg_new_worker(const struct msg *, void *_ctx)
{
	struct msg_context *ctx = (struct msg_context *)_ctx;
	return server_new_worker(ctx->server, ctx->client_fd);
}

static int msg_ci_run(const struct msg *msg, void *_ctx)
{
	struct msg_context *ctx = (struct msg_context *)_ctx;

	if (msg->argc != 3) {
		print_error("Invalid number of arguments for a message: %d\n", msg->argc);
		return -1;
	}

	return server_ci_run(ctx->server, msg->argv[1], msg->argv[2]);
}

static int server_msg_handler(const struct msg *msg, void *ctx)
{
	if (msg->argc == 0) {
		print_error("Received an empty message\n");
		return -1;
	}

	if (!strcmp(msg->argv[0], "new_worker"))
		return msg_new_worker(msg, ctx);
	if (!strcmp(msg->argv[0], "ci_run"))
		return msg_ci_run(msg, ctx);

	return msg_dump_unknown(msg);
}

static int server_conn_handler(int fd, void *server)
{
	struct msg_context ctx = {server, fd};
	return msg_recv_and_handle(fd, server_msg_handler, &ctx);
}

static int server_set_stopping(struct server *server)
{
	int ret = 0;

	ret = pthread_mutex_lock(&server->server_mtx);
	if (ret) {
		pthread_print_errno(ret, "pthread_mutex_lock");
		return ret;
	}

	server->stopping = 1;

	ret = pthread_cond_signal(&server->server_cv);
	if (ret) {
		pthread_print_errno(ret, "pthread_cond_signal");
		goto unlock;
	}

unlock:
	pthread_check(pthread_mutex_unlock(&server->server_mtx), "pthread_mutex_unlock");

	return ret;
}

int server_main(struct server *server)
{
	int ret = 0;

	while (!global_stop_flag) {
		ret = tcp_server_accept(&server->tcp_server, server_conn_handler, server);
		if (ret < 0)
			break;
	}

	return server_set_stopping(server);
}

int server_new_worker(struct server *server, int fd)
{
	struct worker_queue_entry *entry;
	int ret = 0;

	print_log("Registering a new worker\n");

	ret = pthread_mutex_lock(&server->server_mtx);
	if (ret) {
		pthread_print_errno(ret, "pthread_mutex_lock");
		return ret;
	}

	ret = worker_queue_entry_create(&entry, fd);
	if (ret < 0)
		goto unlock;

	worker_queue_push(&server->worker_queue, entry);

	ret = pthread_cond_signal(&server->server_cv);
	if (ret) {
		pthread_print_errno(ret, "pthread_cond_signal");
		goto unlock;
	}

unlock:
	pthread_check(pthread_mutex_unlock(&server->server_mtx), "pthread_mutex_unlock");

	return ret;
}

int server_ci_run(struct server *server, const char *url, const char *rev)
{
	struct ci_queue_entry *entry;
	int ret = 0;

	print_log("Scheduling a new CI run for repository %s\n", url);

	ret = pthread_mutex_lock(&server->server_mtx);
	if (ret) {
		pthread_print_errno(ret, "pthread_mutex_lock");
		return ret;
	}

	ret = ci_queue_entry_create(&entry, url, rev);
	if (ret < 0)
		goto unlock;

	ci_queue_push(&server->ci_queue, entry);

	ret = pthread_cond_signal(&server->server_cv);
	if (ret) {
		pthread_print_errno(ret, "pthread_cond_signal");
		goto unlock;
	}

unlock:
	pthread_check(pthread_mutex_unlock(&server->server_mtx), "pthread_mutex_unlock");

	return ret;
}
