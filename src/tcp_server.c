/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "tcp_server.h"
#include "compiler.h"
#include "event_loop.h"
#include "log.h"
#include "net.h"
#include "signal.h"

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>

struct tcp_server {
	int fd;
	tcp_server_conn_handler conn_handler;
	void *arg;
};

int tcp_server_create(struct tcp_server **_server, const char *port,
                      tcp_server_conn_handler conn_handler, void *arg)
{
	int ret = 0;

	struct tcp_server *server = calloc(1, sizeof(struct tcp_server));
	if (!server) {
		log_errno("malloc");
		return -1;
	}

	server->conn_handler = conn_handler;
	server->arg = arg;

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
	net_close(server->fd);
	free(server);
}

struct child_context {
	int fd;
	tcp_server_conn_handler conn_handler;
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

	ctx->conn_handler(ctx->fd, ctx->arg);

free_ctx:
	free(ctx);

	return NULL;
}

static int create_connection_thread(int fd, tcp_server_conn_handler conn_handler, void *arg)
{
	sigset_t old_mask;
	pthread_t child;
	int ret = 0;

	struct child_context *ctx = calloc(1, sizeof(*ctx));
	if (!ctx) {
		log_errno("calloc");
		return -1;
	}

	ctx->fd = fd;
	ctx->conn_handler = conn_handler;
	ctx->arg = arg;

	/* Block all signals (we'll unblock them later); the child thread will
	 * have all signals blocked initially. This allows the main thread to
	 * handle SIGINT/SIGTERM/etc. */
	ret = signal_block_all(&old_mask);
	if (ret < 0)
		goto free_ctx;

	ret = pthread_create(&child, NULL, connection_thread, ctx);
	if (ret) {
		pthread_errno(ret, "pthread_create");
		goto restore_mask;
	}

restore_mask:
	/* Restore the previously-enabled signals for handling in the main thread. */
	signal_restore(&old_mask);

	return ret;

free_ctx:
	free(ctx);

	return ret;
}

int tcp_server_accept(const struct tcp_server *server)
{
	int fd = -1, ret = 0;

	ret = net_accept(server->fd);
	if (ret < 0)
		return ret;
	fd = ret;

	ret = create_connection_thread(fd, server->conn_handler, server->arg);
	if (ret < 0)
		goto close_conn;

	return ret;

close_conn:
	net_close(fd);

	return ret;
}

static int tcp_server_event_loop_handler(UNUSED struct event_loop *loop, UNUSED int fd,
                                         UNUSED short revents, void *_server)
{
	struct tcp_server *server = (struct tcp_server *)_server;
	return tcp_server_accept(server);
}

int tcp_server_add_to_event_loop(struct tcp_server *server, struct event_loop *loop)
{
	struct event_fd entry = {
	    .fd = server->fd,
	    .events = POLLIN,
	    .handler = tcp_server_event_loop_handler,
	    .arg = server,
	};
	return event_loop_add(loop, &entry);
}
