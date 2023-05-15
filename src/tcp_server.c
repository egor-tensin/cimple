/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "tcp_server.h"
#include "log.h"
#include "net.h"
#include "signal.h"

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

struct tcp_server {
	int fd;
};

int tcp_server_create(struct tcp_server **_server, const char *port)
{
	int ret = 0;

	struct tcp_server *server = malloc(sizeof(struct tcp_server));
	if (!server) {
		log_errno("malloc");
		return -1;
	}

	ret = net_bind(port);
	if (ret < 0)
		goto free;
	server->fd = ret;

	*_server = server;
	return ret;

free:
	free(server);

	return ret;
}

void tcp_server_destroy(struct tcp_server *server)
{
	log_errno_if(close(server->fd), "close");
	free(server);
}

struct child_context {
	int fd;
	tcp_server_conn_handler handler;
	void *arg;
};

static void *connection_thread(void *_ctx)
{
	struct child_context *ctx = (struct child_context *)_ctx;
	int ret = 0;

	/* Let the child thread handle its signals except those that should be
	 * handled in the main thread. */
	ret = signal_block_stops();
	if (ret < 0)
		goto free_ctx;

	ctx->handler(ctx->fd, ctx->arg);

free_ctx:
	free(ctx);

	return NULL;
}

int tcp_server_accept(const struct tcp_server *server, tcp_server_conn_handler handler, void *arg)
{
	sigset_t old_mask;
	pthread_t child;
	int ret = 0;

	struct child_context *ctx = calloc(1, sizeof(*ctx));
	if (!ctx) {
		log_errno("malloc");
		return -1;
	}

	ctx->handler = handler;
	ctx->arg = arg;

	ret = net_accept(server->fd);
	if (ret < 0)
		goto free_ctx;
	ctx->fd = ret;

	/* Block all signals (we'll unblock them later); the child thread will
	 * have all signals blocked initially. This allows the main thread to
	 * handle SIGINT/SIGTERM/etc. */
	ret = signal_block_all(&old_mask);
	if (ret < 0)
		goto close_conn;

	ret = pthread_create(&child, NULL, connection_thread, ctx);
	if (ret) {
		pthread_errno(ret, "pthread_create");
		goto restore_mask;
	}

	/* Restore the previously-enabled signals for handling in the main thread. */
	signal_restore(&old_mask);

	return ret;

restore_mask:
	signal_restore(&old_mask);

close_conn:
	log_errno_if(close(ctx->fd), "close");

free_ctx:
	free(ctx);

	return ret;
}
