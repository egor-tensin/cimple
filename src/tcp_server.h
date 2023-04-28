/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

struct tcp_server;

int tcp_server_create(struct tcp_server **, const char *port);
void tcp_server_destroy(struct tcp_server *);

typedef int (*tcp_server_conn_handler)(int conn_fd, void *arg);
int tcp_server_accept(const struct tcp_server *, tcp_server_conn_handler, void *arg);

#endif
