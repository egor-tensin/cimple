#ifndef __NET_H__
#define __NET_H__

#include <stdlib.h>

int bind_to_port(const char *port);

typedef int (*connection_handler)(int fd, void *arg);
int accept_connection(int fd, connection_handler, void *arg);

int connect_to_host(const char *host, const char *port);

int send_all(int fd, const void *, size_t);
int send_buf(int fd, const void *, size_t);

int recv_all(int fd, void *, size_t);
int recv_buf(int fd, void **, size_t *);
int recv_static(int fd, void *, size_t);

#endif
