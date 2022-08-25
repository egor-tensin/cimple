#ifndef __SERVER_H__
#define __SERVER_H__

#include "tcp_server.h"

struct settings {
	const char *port;
};

struct server {
	struct tcp_server tcp_server;
};

int server_create(struct server *, const struct settings *);
void server_destroy(const struct server *);

int server_main(const struct server *);

#endif
