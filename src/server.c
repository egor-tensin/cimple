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
#include "log.h"
#include "msg.h"
#include "run_queue.h"
#include "signal.h"
#include "storage.h"
#include "storage_sqlite.h"
#include "tcp_server.h"

#include <pthread.h>
#include <stdlib.h>

struct server {
	pthread_mutex_t server_mtx;
	pthread_cond_t server_cv;

	int stopping;

	struct storage storage;

	struct tcp_server *tcp_server;

	struct run_queue run_queue;
};

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

	run_queue_create(&server->run_queue);

	*_server = server;
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

	run_queue_destroy(&server->run_queue);
	tcp_server_destroy(server->tcp_server);
	storage_destroy(&server->storage);
	pthread_errno_if(pthread_cond_destroy(&server->server_cv), "pthread_cond_destroy");
	pthread_errno_if(pthread_mutex_destroy(&server->server_mtx), "pthread_mutex_destroy");
	free(server);
}

static int server_has_runs(const struct server *server)
{
	return !run_queue_is_empty(&server->run_queue);
}

static int worker_ci_run(int fd, const struct run_queue_entry *ci_run)
{
	struct msg *request = NULL, *response = NULL;
	int ret = 0;

	const char *argv[] = {CMD_RUN, run_queue_entry_get_url(ci_run),
	                      run_queue_entry_get_rev(ci_run), NULL};

	ret = msg_from_argv(&request, argv);
	if (ret < 0)
		return ret;

	ret = msg_send_and_wait(fd, request, &response);
	msg_free(request);
	if (ret < 0)
		return ret;

	if (!msg_is_success(response)) {
		log_err("Failed to schedule a CI run: worker is busy?\n");
		msg_dump(response);
		goto free_response;
	}

	/* TODO: handle the response. */

free_response:
	msg_free(response);

	return ret;
}

static int worker_dequeue_run(struct server *server, struct run_queue_entry **ci_run)
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

	*ci_run = run_queue_remove_first(&server->run_queue);
	log("Removed a CI run for repository %s from the queue\n",
	    run_queue_entry_get_url(*ci_run));
	goto unlock;

unlock:
	pthread_errno_if(pthread_mutex_unlock(&server->server_mtx), "pthread_mutex_unlock");

	return ret;
}

static int worker_requeue_run(struct server *server, struct run_queue_entry *ci_run)
{
	int ret = 0;

	ret = pthread_mutex_lock(&server->server_mtx);
	if (ret) {
		pthread_errno(ret, "pthread_mutex_lock");
		return ret;
	}

	run_queue_add_first(&server->run_queue, ci_run);
	log("Requeued a CI run for repository %s\n", run_queue_entry_get_url(ci_run));

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
	struct run_queue_entry *ci_run = NULL;
	int ret = 0;

	ret = worker_dequeue_run(server, &ci_run);
	if (ret < 0)
		return ret;

	ret = worker_ci_run(fd, ci_run);
	if (ret < 0)
		goto requeue_run;

	run_queue_entry_destroy(ci_run);
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

static int msg_new_worker_handler(int client_fd, UNUSED const struct msg *request, void *_server,
                                  UNUSED struct msg **response)
{
	return worker_thread((struct server *)_server, client_fd);
}

static int msg_ci_run_queue(struct server *server, const char *url, const char *rev)
{
	struct run_queue_entry *entry = NULL;
	int ret = 0;

	ret = pthread_mutex_lock(&server->server_mtx);
	if (ret) {
		pthread_errno(ret, "pthread_mutex_lock");
		return ret;
	}

	ret = run_queue_entry_create(&entry, url, rev);
	if (ret < 0)
		goto unlock;

	run_queue_add_last(&server->run_queue, entry);
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

static int msg_ci_run_handler(UNUSED int client_fd, const struct msg *request, void *_server,
                              struct msg **response)
{
	struct server *server = (struct server *)_server;
	int ret = 0;

	if (msg_get_length(request) != 3) {
		log_err("Invalid number of arguments for a message: %zu\n",
		        msg_get_length(request));
		msg_dump(request);
		return -1;
	}

	const char **words = msg_get_words(request);

	ret = msg_ci_run_queue(server, words[1], words[2]);
	if (ret < 0)
		return ret;

	return msg_success(response);
}

static struct cmd_desc commands[] = {
    {CMD_NEW_WORKER, msg_new_worker_handler},
    {CMD_RUN, msg_ci_run_handler},
};

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
	struct cmd_dispatcher *dispatcher = NULL;
	int ret = 0;

	ret = signal_install_global_handler();
	if (ret < 0)
		return ret;

	ret = cmd_dispatcher_create(&dispatcher, commands, sizeof(commands) / sizeof(commands[0]),
	                            server);
	if (ret < 0)
		return ret;

	while (!global_stop_flag) {
		log("Waiting for new connections\n");

		ret = tcp_server_accept(server->tcp_server, cmd_dispatcher_handle_conn, dispatcher);
		if (ret < 0) {
			if (errno == EINVAL && global_stop_flag)
				ret = 0;
			goto dispatcher_destroy;
		}
	}

dispatcher_destroy:
	cmd_dispatcher_destroy(dispatcher);

	return server_set_stopping(server);
}
