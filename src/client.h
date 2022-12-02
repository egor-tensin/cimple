/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __CLIENT_H__
#define __CLIENT_H__

struct settings {
	const char *host;
	const char *port;
};

struct client {
	int fd;
};

int client_create(struct client *, const struct settings *);
void client_destroy(const struct client *);

int client_main(const struct client *, int argc, char *argv[]);

#endif
