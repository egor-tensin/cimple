#include "server.h"
#include "ci_queue.h"
#include "log.h"
#include "msg.h"
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

	ret = pthread_mutex_lock(&server->scheduler_mtx);
	if (ret < 0) {
		print_errno("pthread_mutex_lock");
		goto exit;
	}

	while (1) {
		while (!server_has_runs_and_workers(server)) {
			ret = pthread_cond_wait(&server->scheduler_cv, &server->scheduler_mtx);
			if (ret < 0) {
				print_errno("pthread_cond_wait");
				goto unlock;
			}
		}

		ret = server_scheduler_iteration(server);
		if (ret < 0)
			goto unlock;
	}

unlock:
	if (pthread_mutex_unlock(&server->scheduler_mtx))
		print_errno("pthread_mutex_unlock");

exit:
	return NULL;
}

int server_create(struct server *server, const struct settings *settings)
{
	int ret = 0;

	ret = tcp_server_create(&server->tcp_server, settings->port);
	if (ret < 0)
		return ret;

	worker_queue_create(&server->worker_queue);

	ci_queue_create(&server->ci_queue);

	ret = pthread_mutex_init(&server->scheduler_mtx, NULL);
	if (ret < 0) {
		print_errno("pthread_mutex_init");
		goto destroy_tcp_server;
	}

	ret = pthread_cond_init(&server->scheduler_cv, NULL);
	if (ret < 0) {
		print_errno("pthread_cond_init");
		goto destroy_scheduler_mtx;
	}

	ret = pthread_create(&server->scheduler, NULL, server_scheduler, server);
	if (ret < 0) {
		print_errno("pthread_create");
		goto destroy_scheduler_cv;
	}

	return ret;

destroy_scheduler_cv:
	if (pthread_cond_destroy(&server->scheduler_cv))
		print_errno("pthread_cond_destroy");

destroy_scheduler_mtx:
	if (pthread_mutex_destroy(&server->scheduler_mtx))
		print_errno("pthread_mutex_destroy");

	ci_queue_destroy(&server->ci_queue);

	worker_queue_destroy(&server->worker_queue);

destroy_tcp_server:
	tcp_server_destroy(&server->tcp_server);

	return ret;
}

void server_destroy(struct server *server)
{
	if (pthread_join(server->scheduler, NULL))
		print_errno("pthread_join");
	if (pthread_cond_destroy(&server->scheduler_cv))
		print_errno("pthread_cond_destroy");
	if (pthread_mutex_destroy(&server->scheduler_mtx))
		print_errno("pthread_mutex_destroy");
	ci_queue_destroy(&server->ci_queue);
	worker_queue_destroy(&server->worker_queue);
	tcp_server_destroy(&server->tcp_server);
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

int server_main(struct server *server)
{
	int ret = 0;

	while (1) {
		ret = tcp_server_accept(&server->tcp_server, server_conn_handler, server);
		if (ret < 0)
			return ret;
	}
}

int server_new_worker(struct server *server, int fd)
{
	struct worker_queue_entry *entry;
	int ret = 0;

	print_log("Registering a new worker\n");

	ret = pthread_mutex_lock(&server->scheduler_mtx);
	if (ret < 0) {
		print_errno("pthread_mutex_lock");
		return ret;
	}

	ret = worker_queue_entry_create(&entry, fd);
	if (ret < 0)
		goto unlock;

	worker_queue_push(&server->worker_queue, entry);

	ret = pthread_cond_signal(&server->scheduler_cv);
	if (ret < 0) {
		print_errno("pthread_cond_signal");
		goto unlock;
	}

unlock:
	if (pthread_mutex_unlock(&server->scheduler_mtx))
		print_errno("pthread_mutex_unlock");

	return ret;
}

int server_ci_run(struct server *server, const char *url, const char *rev)
{
	struct ci_queue_entry *entry;
	int ret = 0;

	print_log("Scheduling a new CI run for repository %s\n", url);

	ret = pthread_mutex_lock(&server->scheduler_mtx);
	if (ret < 0) {
		print_errno("pthread_mutex_lock");
		return ret;
	}

	ret = ci_queue_entry_create(&entry, url, rev);
	if (ret < 0)
		goto unlock;

	ci_queue_push(&server->ci_queue, entry);

	ret = pthread_cond_signal(&server->scheduler_cv);
	if (ret < 0) {
		print_errno("pthread_cond_signal");
		goto unlock;
	}

unlock:
	if (pthread_mutex_unlock(&server->scheduler_mtx))
		print_errno("pthread_mutex_unlock");

	return ret;
}
