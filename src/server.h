/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __SERVER_H__
#define __SERVER_H__

#include "ci_queue.h"
#include "storage.h"
#include "tcp_server.h"

#include <pthread.h>

struct settings {
	const char *port;

	const char *sqlite_path;
};

struct server;

int server_create(struct server **, const struct settings *);
void server_destroy(struct server *);

int server_main(struct server *);

#endif
