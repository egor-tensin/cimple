/*
 * Copyright (c) 2022 Egor Tensin <egor@tensin.name>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __NET_H__
#define __NET_H__

#include "buf.h"

#include <stddef.h>

int net_bind(const char *port);
int net_accept(int fd);
int net_connect(const char *host, const char *port);
void net_close(int fd);

int net_send(int fd, const void *, size_t);
int net_recv(int fd, void *, size_t);

int net_send_buf(int fd, const struct buf *);
int net_recv_buf(int fd, struct buf **);

#endif
