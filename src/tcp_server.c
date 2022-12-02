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

int tcp_server_create(struct tcp_server *server, const char *port)
{
	server->fd = net_bind(port);
	if (server->fd < 0)
		return server->fd;

	return 0;
}

void tcp_server_destroy(const struct tcp_server *server)
{
	log_errno_if(close(server->fd), "close");
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

	ret = signal_block_child();
	if (ret < 0)
		goto close;

	ctx->handler(ctx->fd, ctx->arg);

close:
	log_errno_if(close(ctx->fd), "close");
	free(ctx);
	return NULL;
}

int tcp_server_accept(const struct tcp_server *server, tcp_server_conn_handler handler, void *arg)
{
	struct child_context *ctx;
	pthread_attr_t child_attr;
	sigset_t old_mask;
	pthread_t child;
	int conn_fd, ret = 0;

	ret = net_accept(server->fd);
	if (ret < 0)
		return ret;
	conn_fd = ret;

	ctx = malloc(sizeof(*ctx));
	if (!ctx) {
		log_errno("malloc");
		ret = -1;
		goto close_conn;
	}
	*ctx = (struct child_context){conn_fd, handler, arg};

	ret = pthread_attr_init(&child_attr);
	if (ret) {
		pthread_errno(ret, "pthread_attr_init");
		goto free_ctx;
	}

	ret = pthread_attr_setdetachstate(&child_attr, PTHREAD_CREATE_DETACHED);
	if (ret) {
		pthread_errno(ret, "pthread_attr_setdetachstate");
		goto destroy_attr;
	}

	ret = signal_block_parent(&old_mask);
	if (ret < 0)
		goto destroy_attr;

	ret = pthread_create(&child, &child_attr, connection_thread, ctx);
	if (ret) {
		pthread_errno(ret, "pthread_create");
		goto restore_mask;
	}

	signal_set(&old_mask, NULL);

	pthread_errno_if(pthread_attr_destroy(&child_attr), "pthread_attr_destroy");

	return ret;

restore_mask:
	signal_set(&old_mask, NULL);

destroy_attr:
	pthread_errno_if(pthread_attr_destroy(&child_attr), "pthread_attr_destroy");

free_ctx:
	free(ctx);

close_conn:
	log_errno_if(close(conn_fd), "close");

	return ret;
}
