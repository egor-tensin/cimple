#ifndef __SERVER_H__
#define __SERVER_H__

#include "ci_queue.h"
#include "tcp_server.h"
#include "worker_queue.h"

#include <pthread.h>

struct settings {
	const char *port;
};

struct server {
	struct tcp_server tcp_server;

	struct worker_queue worker_queue;
	struct ci_queue ci_queue;

	pthread_mutex_t scheduler_mtx;
	pthread_cond_t scheduler_cv;
	pthread_t scheduler;
};

int server_create(struct server *, const struct settings *);
void server_destroy(struct server *);

int server_main(struct server *);

int server_new_worker(struct server *, int fd);
int server_ci_run(struct server *, const char *url, const char *rev);

#endif
