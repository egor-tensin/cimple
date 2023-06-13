/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "server.h"
#include "command.h"
#include "compiler.h"
#include "const.h"
#include "event_loop.h"
#include "log.h"
#include "msg.h"
#include "run_queue.h"
#include "signal.h"
#include "storage.h"
#include "storage_sqlite.h"
#include "tcp_server.h"
#include "worker_queue.h"

#include <poll.h>
#include <pthread.h>
#include <stdlib.h>

struct server {
	pthread_mutex_t server_mtx;
	pthread_cond_t server_cv;

	int stopping;

	struct cmd_dispatcher *cmd_dispatcher;

	struct event_loop *event_loop;
	int signalfd;

	struct worker_queue worker_queue;
	struct run_queue run_queue;

	struct storage storage;

	pthread_t main_thread;

	struct tcp_server *tcp_server;
};

static int server_lock(struct server *server)
{
	int ret = pthread_mutex_lock(&server->server_mtx);
	if (ret) {
		pthread_errno(ret, "pthread_mutex_lock");
		return ret;
	}
	return ret;
}

static void server_unlock(struct server *server)
{
	pthread_errno_if(pthread_mutex_unlock(&server->server_mtx), "pthread_mutex_unlock");
}

static int server_wait(struct server *server)
{
	int ret = pthread_cond_wait(&server->server_cv, &server->server_mtx);
	if (ret) {
		pthread_errno(ret, "pthread_cond_wait");
		return ret;
	}
	return ret;
}

static void server_notify(struct server *server)
{
	pthread_errno_if(pthread_cond_signal(&server->server_cv), "pthread_cond_signal");
}

static int server_set_stopping(UNUSED struct event_loop *loop, UNUSED int fd, UNUSED short revents,
                               void *_server)
{
	struct server *server = (struct server *)_server;
	int ret = 0;

	ret = server_lock(server);
	if (ret < 0)
		return ret;

	server->stopping = 1;

	server_notify(server);
	server_unlock(server);
	return ret;
}

static int server_has_workers(const struct server *server)
{
	return !worker_queue_is_empty(&server->worker_queue);
}

static int server_enqueue_worker(struct server *server, struct worker *worker)
{
	int ret = 0;

	ret = server_lock(server);
	if (ret < 0)
		return ret;

	worker_queue_add_last(&server->worker_queue, worker);
	log("Added a new worker %d to the queue\n", worker_get_fd(worker));

	server_notify(server);
	server_unlock(server);
	return ret;
}

static int server_has_runs(const struct server *server)
{
	return !run_queue_is_empty(&server->run_queue);
}

static int server_enqueue_run(struct server *server, struct run *run)
{
	int ret = 0;

	ret = server_lock(server);
	if (ret < 0)
		return ret;

	run_queue_add_last(&server->run_queue, run);
	log("Added a new run for repository %s to the queue\n", run_get_url(run));

	server_notify(server);
	server_unlock(server);
	return ret;
}

static int server_ready_for_action(const struct server *server)
{
	return server->stopping || (server_has_runs(server) && server_has_workers(server));
}

static int server_wait_for_action(struct server *server)
{
	int ret = 0;

	while (!server_ready_for_action(server)) {
		ret = server_wait(server);
		if (ret < 0)
			return ret;
	}

	return ret;
}

static void server_assign_run(struct server *server)
{
	struct run *run = run_queue_remove_first(&server->run_queue);
	log("Removed run for repository %s from the queue\n", run_get_url(run));

	struct worker *worker = worker_queue_remove_first(&server->worker_queue);
	log("Removed worker %d from the queue\n", worker_get_fd(worker));

	const char *argv[] = {CMD_RUN, run_get_url(run), run_get_rev(run), NULL};
	int ret = msg_talk_argv(worker_get_fd(worker), argv, NULL);

	if (ret < 0) {
		log("Failed to assign run for repository %s to worker %d, requeueing\n",
		    run_get_url(run), worker_get_fd(worker));
		run_queue_add_first(&server->run_queue, run);
	} else {
		log("Assigned run for repository %s to worker %d\n", run_get_url(run),
		    worker_get_fd(worker));
		run_destroy(run);
	}

	worker_destroy(worker);
}

static void *server_main_thread(void *_server)
{
	struct server *server = (struct server *)_server;
	int ret = 0;

	ret = server_lock(server);
	if (ret < 0)
		goto exit;

	while (1) {
		ret = server_wait_for_action(server);
		if (ret < 0)
			goto unlock;

		if (server->stopping)
			goto unlock;

		server_assign_run(server);
	}

unlock:
	server_unlock(server);

exit:
	return NULL;
}

static int server_handle_cmd_new_worker(UNUSED const struct msg *request,
                                        UNUSED struct msg **response, void *_ctx)
{
	struct cmd_conn_ctx *ctx = (struct cmd_conn_ctx *)_ctx;
	struct server *server = (struct server *)ctx->arg;
	int client_fd = ctx->fd;

	struct worker *worker = NULL;
	int ret = 0;

	ret = worker_create(&worker, client_fd);
	if (ret < 0)
		return ret;

	ret = server_enqueue_worker(server, worker);
	if (ret < 0)
		goto destroy_worker;

	return ret;

destroy_worker:
	worker_destroy(worker);

	return ret;
}

static int server_handle_cmd_run(const struct msg *request, struct msg **response, void *_ctx)
{
	struct cmd_conn_ctx *ctx = (struct cmd_conn_ctx *)_ctx;
	struct server *server = (struct server *)ctx->arg;
	struct run *run = NULL;

	int ret = 0;

	ret = run_from_msg(&run, request);
	if (ret < 0)
		return ret;

	ret = msg_success(response);
	if (ret < 0)
		goto destroy_run;

	ret = server_enqueue_run(server, run);
	if (ret < 0)
		goto free_response;

	return ret;

free_response:
	msg_free(*response);
	*response = NULL;

destroy_run:
	run_destroy(run);

	return ret;
}

static int server_handle_cmd_complete(UNUSED const struct msg *request,
                                      UNUSED struct msg **response, void *_ctx)
{
	struct cmd_conn_ctx *ctx = (struct cmd_conn_ctx *)_ctx;
	int client_fd = ctx->fd;
	int ret = 0;

	log("Received a \"run complete\" message from worker %d\n", client_fd);

	return ret;
}

static struct cmd_desc commands[] = {
    {CMD_NEW_WORKER, server_handle_cmd_new_worker},
    {CMD_RUN, server_handle_cmd_run},
    {CMD_COMPLETE, server_handle_cmd_complete},
};

static const size_t numof_commands = sizeof(commands) / sizeof(commands[0]);

int server_create(struct server **_server, const struct settings *settings)
{
	struct storage_settings storage_settings;
	int ret = 0;

	struct server *server = malloc(sizeof(struct server));
	if (!server) {
		log_errno("malloc");
		return -1;
	}

	ret = pthread_mutex_init(&server->server_mtx, NULL);
	if (ret) {
		pthread_errno(ret, "pthread_mutex_init");
		goto free;
	}

	ret = pthread_cond_init(&server->server_cv, NULL);
	if (ret) {
		pthread_errno(ret, "pthread_cond_init");
		goto destroy_mtx;
	}

	server->stopping = 0;

	ret = cmd_dispatcher_create(&server->cmd_dispatcher, commands, numof_commands, server);
	if (ret < 0)
		goto destroy_cv;

	ret = event_loop_create(&server->event_loop);
	if (ret < 0)
		goto destroy_cmd_dispatcher;

	ret = signalfd_create_sigterms();
	if (ret < 0)
		goto destroy_event_loop;
	server->signalfd = ret;

	ret = event_loop_add(server->event_loop, server->signalfd, POLLIN, server_set_stopping,
	                     server);
	if (ret < 0)
		goto close_signalfd;

	worker_queue_create(&server->worker_queue);
	run_queue_create(&server->run_queue);

	ret = storage_settings_create_sqlite(&storage_settings, settings->sqlite_path);
	if (ret < 0)
		goto destroy_run_queue;

	ret = storage_create(&server->storage, &storage_settings);
	storage_settings_destroy(&storage_settings);
	if (ret < 0)
		goto destroy_run_queue;

	ret = tcp_server_create(&server->tcp_server, settings->port, cmd_dispatcher_handle_conn,
	                        server->cmd_dispatcher);
	if (ret < 0)
		goto destroy_storage;

	ret = tcp_server_add_to_event_loop(server->tcp_server, server->event_loop);
	if (ret < 0)
		goto destroy_tcp_server;

	ret = pthread_create(&server->main_thread, NULL, server_main_thread, server);
	if (ret) {
		pthread_errno(ret, "pthread_create");
		goto destroy_tcp_server;
	}

	*_server = server;
	return ret;

destroy_tcp_server:
	tcp_server_destroy(server->tcp_server);

destroy_storage:
	storage_destroy(&server->storage);

destroy_run_queue:
	run_queue_destroy(&server->run_queue);

	worker_queue_destroy(&server->worker_queue);

close_signalfd:
	signalfd_destroy(server->signalfd);

destroy_event_loop:
	event_loop_destroy(server->event_loop);

destroy_cmd_dispatcher:
	cmd_dispatcher_destroy(server->cmd_dispatcher);

destroy_cv:
	pthread_errno_if(pthread_cond_destroy(&server->server_cv), "pthread_cond_destroy");

destroy_mtx:
	pthread_errno_if(pthread_mutex_destroy(&server->server_mtx), "pthread_mutex_destroy");

free:
	free(server);

	return ret;
}

void server_destroy(struct server *server)
{
	log("Shutting down\n");

	pthread_errno_if(pthread_join(server->main_thread, NULL), "pthread_join");
	tcp_server_destroy(server->tcp_server);
	storage_destroy(&server->storage);
	run_queue_destroy(&server->run_queue);
	worker_queue_destroy(&server->worker_queue);
	signalfd_destroy(server->signalfd);
	event_loop_destroy(server->event_loop);
	cmd_dispatcher_destroy(server->cmd_dispatcher);
	pthread_errno_if(pthread_cond_destroy(&server->server_cv), "pthread_cond_destroy");
	pthread_errno_if(pthread_mutex_destroy(&server->server_mtx), "pthread_mutex_destroy");
	free(server);
}

static int server_listen_thread(struct server *server)
{
	int ret = 0;

	while (!server->stopping) {
		log("Waiting for new connections\n");

		ret = event_loop_run(server->event_loop);
		if (ret < 0)
			return ret;
	}

	return 0;
}

int server_main(struct server *server)
{
	return server_listen_thread(server);
}
