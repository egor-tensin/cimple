/*
 * Copyright (c) 2022 Egor Tensin <egor@tensin.name>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __WORKER_H__
#define __WORKER_H__

struct settings {
	const char *host;
	const char *port;
};

struct worker;

int worker_create(struct worker **, const struct settings *);
void worker_destroy(struct worker *);

int worker_main(struct worker *);

#endif
