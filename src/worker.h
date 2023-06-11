/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __WORKER_H__
#define __WORKER_H__

struct settings {
	char *host;
	char *port;
};

struct worker;

int worker_create(struct worker **, const struct settings *);
void worker_destroy(struct worker *);

int worker_main(struct worker *);

#endif
