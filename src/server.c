/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "server.h"
#include "ci_queue.h"
#include "compiler.h"
#include "const.h"
#include "log.h"
#include "msg.h"
#include "signal.h"
#include "storage.h"
#include "storage_sqlite.h"
#include "tcp_server.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

struct server {
	pthread_mutex_t server_mtx;
	pthread_cond_t server_cv;

	int stopping;

	struct storage storage;

	struct tcp_server *tcp_server;

	struct ci_queue ci_queue;
};

int server_create(struct server **_server, const struct settings *settings)
{
	struct server *server;
	struct storage_settings storage_settings;
	int ret = 0;

	*_server = malloc(sizeof(struct server));
	if (!*_server) {
		log_errno("malloc");
		return -1;
	}
	server = *_server;

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

	ret = storage_settings_create_sqlite(&storage_settings, settings->sqlite_path);
	if (ret < 0)
		goto destroy_cv;

	ret = storage_create(&server->storage, &storage_settings);
	storage_settings_destroy(&storage_settings);
	if (ret < 0)
		goto destroy_cv;

	ret = tcp_server_create(&server->tcp_server, settings->port);
	if (ret < 0)
		goto destroy_storage;

	ci_queue_create(&server->ci_queue);

	return ret;

destroy_storage:
	storage_destroy(&server->storage);

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

	ci_queue_destroy(&server->ci_queue);
	tcp_server_destroy(server->tcp_server);
	storage_destroy(&server->storage);
	pthread_errno_if(pthread_cond_destroy(&server->server_cv), "pthread_cond_destroy");
	pthread_errno_if(pthread_mutex_destroy(&server->server_mtx), "pthread_mutex_destroy");
	free(server);
}

static int server_has_runs(const struct server *server)
{
	return !ci_queue_is_empty(&server->ci_queue);
}

static int worker_ci_run(int fd, const struct ci_queue_entry *ci_run)
{
	struct msg request, response;
	int ret = 0;

	char *argv[] = {CMD_CI_RUN, ci_run->url, ci_run->rev, NULL};

	ret = msg_from_argv(&request, argv);
	if (ret < 0)
		return ret;

	ret = msg_send_and_wait(fd, &request, &response);
	msg_free(&request);
	if (ret < 0)
		return ret;

	if (response.argc < 0) {
		log_err("Failed to schedule a CI run: worker is busy?\n");
		ret = -1;
		goto free_response;
	}

	/* TODO: handle the response. */

free_response:
	msg_free(&response);

	return ret;
}

static int worker_dequeue_run(struct server *server, struct ci_queue_entry **ci_run)
{
	int ret = 0;

	ret = pthread_mutex_lock(&server->server_mtx);
	if (ret) {
		pthread_errno(ret, "pthread_mutex_lock");
		return ret;
	}

	while (!server->stopping && !server_has_runs(server)) {
		ret = pthread_cond_wait(&server->server_cv, &server->server_mtx);
		if (ret) {
			pthread_errno(ret, "pthread_cond_wait");
			goto unlock;
		}
	}

	if (server->stopping) {
		ret = -1;
		goto unlock;
	}

	*ci_run = ci_queue_pop(&server->ci_queue);
	log("Removed a CI run for repository %s from the queue\n", (*ci_run)->url);
	goto unlock;

unlock:
	pthread_errno_if(pthread_mutex_unlock(&server->server_mtx), "pthread_mutex_unlock");

	return ret;
}

static int worker_requeue_run(struct server *server, struct ci_queue_entry *ci_run)
{
	int ret = 0;

	ret = pthread_mutex_lock(&server->server_mtx);
	if (ret) {
		pthread_errno(ret, "pthread_mutex_lock");
		return ret;
	}

	ci_queue_push_head(&server->ci_queue, ci_run);
	log("Requeued a CI run for repository %s\n", ci_run->url);

	ret = pthread_cond_signal(&server->server_cv);
	if (ret) {
		pthread_errno(ret, "pthread_cond_signal");
		ret = 0;
		goto unlock;
	}

unlock:
	pthread_errno_if(pthread_mutex_unlock(&server->server_mtx), "pthread_mutex_unlock");

	return ret;
}

static int worker_iteration(struct server *server, int fd)
{
	struct ci_queue_entry *ci_run = NULL;
	int ret = 0;

	ret = worker_dequeue_run(server, &ci_run);
	if (ret < 0)
		return ret;

	ret = worker_ci_run(fd, ci_run);
	if (ret < 0)
		goto requeue_run;

	ci_queue_entry_destroy(ci_run);
	return ret;

requeue_run:
	worker_requeue_run(server, ci_run);

	return ret;
}

static int worker_thread(struct server *server, int fd)
{
	int ret = 0;

	while (1) {
		ret = worker_iteration(server, fd);
		if (ret < 0)
			return ret;
	}

	return ret;
}

static int msg_new_worker_handler(struct server *server, int client_fd,
                                  UNUSED const struct msg *request)
{
	return worker_thread(server, client_fd);
}

static int msg_new_worker_parser(UNUSED const struct msg *msg)
{
	return 1;
}

static int msg_ci_run_queue(struct server *server, const char *url, const char *rev)
{
	struct ci_queue_entry *entry;
	int ret = 0;

	ret = pthread_mutex_lock(&server->server_mtx);
	if (ret) {
		pthread_errno(ret, "pthread_mutex_lock");
		return ret;
	}

	ret = ci_queue_entry_create(&entry, url, rev);
	if (ret < 0)
		goto unlock;

	ci_queue_push(&server->ci_queue, entry);
	log("Added a new CI run for repository %s to the queue\n", url);

	ret = pthread_cond_signal(&server->server_cv);
	if (ret) {
		pthread_errno(ret, "pthread_cond_signal");
		ret = 0;
		goto unlock;
	}

unlock:
	pthread_errno_if(pthread_mutex_unlock(&server->server_mtx), "pthread_mutex_unlock");

	return ret;
}

static int msg_ci_run_handler(struct server *server, int client_fd, const struct msg *msg)
{
	struct msg response;
	int ret = 0;

	ret = msg_ci_run_queue(server, msg->argv[1], msg->argv[2]);
	if (ret < 0)
		ret = msg_error(&response);
	else
		ret = msg_success(&response);

	if (ret < 0)
		return ret;

	ret = msg_send(client_fd, &response);
	msg_free(&response);
	return ret;
}

static int msg_ci_run_parser(const struct msg *msg)
{
	if (msg->argc != 3) {
		log_err("Invalid number of arguments for a message: %d\n", msg->argc);
		return 0;
	}

	return 1;
}

typedef int (*msg_parser)(const struct msg *msg);
typedef int (*msg_handler)(struct server *, int client_fd, const struct msg *msg);

struct msg_descr {
	const char *cmd;
	msg_parser parser;
	msg_handler handler;
};

struct msg_descr messages[] = {
    {CMD_NEW_WORKER, msg_new_worker_parser, msg_new_worker_handler},
    {CMD_CI_RUN, msg_ci_run_parser, msg_ci_run_handler},
};

static int server_msg_handler(struct server *server, int client_fd, const struct msg *request)
{
	if (request->argc == 0)
		goto unknown_request;

	size_t numof_messages = sizeof(messages) / sizeof(messages[0]);

	for (size_t i = 0; i < numof_messages; ++i) {
		if (strcmp(messages[i].cmd, request->argv[0]))
			continue;
		if (!messages[i].parser(request))
			continue;
		return messages[i].handler(server, client_fd, request);
	}

unknown_request:
	log_err("Received an unknown message\n");
	msg_dump(request);
	struct msg response;
	msg_error(&response);
	return msg_send(client_fd, &response);
}

static int server_conn_handler(int client_fd, void *_server)
{
	struct server *server = (struct server *)_server;
	struct msg request;
	int ret = 0;

	ret = msg_recv(client_fd, &request);
	if (ret < 0)
		return ret;

	ret = server_msg_handler(server, client_fd, &request);
	msg_free(&request);
	return ret;
}

static int server_set_stopping(struct server *server)
{
	int ret = 0;

	ret = pthread_mutex_lock(&server->server_mtx);
	if (ret) {
		pthread_errno(ret, "pthread_mutex_lock");
		return ret;
	}

	server->stopping = 1;

	ret = pthread_cond_broadcast(&server->server_cv);
	if (ret) {
		pthread_errno(ret, "pthread_cond_signal");
		goto unlock;
	}

unlock:
	pthread_errno_if(pthread_mutex_unlock(&server->server_mtx), "pthread_mutex_unlock");

	return ret;
}

int server_main(struct server *server)
{
	int ret = 0;

	while (!global_stop_flag) {
		log("Waiting for new connections\n");

		ret = tcp_server_accept(server->tcp_server, server_conn_handler, server);
		if (ret < 0)
			break;
	}

	return server_set_stopping(server);
}
