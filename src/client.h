/*
 * Copyright (c) 2022 Egor Tensin <egor@tensin.name>
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

struct client;

int client_create(struct client **);
void client_destroy(struct client *);

int client_main(const struct client *, const struct settings *, int argc, const char **argv);

#endif
