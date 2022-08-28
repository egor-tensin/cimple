#ifndef __SERVER_H__
#define __SERVER_H__

#include "ci_queue.h"
#include "tcp_server.h"

#include <pthread.h>

struct settings {
	const char *port;
};

struct server {
	pthread_mutex_t server_mtx;
	pthread_cond_t server_cv;

	int stopping;

	struct tcp_server tcp_server;

	struct ci_queue ci_queue;
};

int server_create(struct server *, const struct settings *);
void server_destroy(struct server *);

int server_main(struct server *);

#endif
