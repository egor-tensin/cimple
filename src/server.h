/*
 * Copyright (c) 2022 Egor Tensin <egor@tensin.name>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __SERVER_H__
#define __SERVER_H__

struct settings {
	const char *port;

	const char *sqlite_path;
};

struct server;

int server_create(struct server **, const struct settings *);
void server_destroy(struct server *);

int server_main(struct server *);

#endif
