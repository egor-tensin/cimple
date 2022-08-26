#include "tcp_server.h"
#include "log.h"
#include "net.h"

#include <pthread.h>
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
	check_errno(close(server->fd), "close");
}

struct child_context {
	int fd;
	tcp_server_conn_handler handler;
	void *arg;
};

static void *connection_thread(void *_ctx)
{
	struct child_context *ctx = (struct child_context *)_ctx;
	ctx->handler(ctx->fd, ctx->arg);
	check_errno(close(ctx->fd), "close");
	free(ctx);
	return NULL;
}

int tcp_server_accept(const struct tcp_server *server, tcp_server_conn_handler handler, void *arg)
{
	struct child_context *ctx;
	pthread_attr_t child_attr;
	pthread_t child;
	int conn_fd, ret = 0;

	ret = net_accept(server->fd);
	if (ret < 0) {
		print_errno("accept");
		return ret;
	}
	conn_fd = ret;

	ctx = malloc(sizeof(*ctx));
	if (!ctx) {
		print_errno("malloc");
		ret = -1;
		goto close_conn;
	}
	*ctx = (struct child_context){conn_fd, handler, arg};

	ret = pthread_attr_init(&child_attr);
	if (ret) {
		pthread_print_errno(ret, "pthread_attr_init");
		goto free_ctx;
	}

	ret = pthread_attr_setdetachstate(&child_attr, PTHREAD_CREATE_DETACHED);
	if (ret) {
		pthread_print_errno(ret, "pthread_attr_setdetachstate");
		goto destroy_attr;
	}

	ret = pthread_create(&child, &child_attr, connection_thread, ctx);
	if (ret) {
		pthread_print_errno(ret, "pthread_create");
		goto destroy_attr;
	}

	pthread_check(pthread_attr_destroy(&child_attr), "pthread_attr_destroy");

	return ret;

destroy_attr:
	pthread_check(pthread_attr_destroy(&child_attr), "pthread_attr_destroy");

free_ctx:
	free(ctx);

close_conn:
	check_errno(close(conn_fd), "close");

	return ret;
}
