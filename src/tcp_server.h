#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

struct tcp_server {
	int fd;
};

int tcp_server_create(struct tcp_server *, const char *port);
void tcp_server_destroy(const struct tcp_server *);

typedef int (*tcp_server_conn_handler)(int conn_fd, void *arg);
int tcp_server_accept(const struct tcp_server *, tcp_server_conn_handler, void *arg);

#endif
