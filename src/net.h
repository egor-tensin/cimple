/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __NET_H__
#define __NET_H__

#include <stdint.h>
#include <stdlib.h>

int net_bind(const char *port);
int net_accept(int fd);
int net_connect(const char *host, const char *port);

int net_send_all(int fd, const void *, size_t);
int net_send_buf(int fd, const void *, uint32_t);

int net_recv_all(int fd, void *, size_t);
int net_recv_buf(int fd, void **, uint32_t *);

#endif
