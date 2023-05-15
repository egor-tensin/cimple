/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __MSG_H__
#define __MSG_H__

#include <stddef.h>

struct msg;

int msg_from_argv(struct msg **, const char **argv);
void msg_free(struct msg *);

int msg_copy(struct msg **, const struct msg *);

size_t msg_get_length(const struct msg *);
const char **msg_get_strings(const struct msg *);
const char *msg_get_first_string(const struct msg *);

int msg_success(struct msg **);
int msg_error(struct msg **);

int msg_is_success(const struct msg *);
int msg_is_error(const struct msg *);

int msg_recv(int fd, struct msg **);
int msg_send(int fd, const struct msg *);

int msg_communicate(int fd, const struct msg *, struct msg **response);
int msg_connect_and_communicate(const char *host, const char *port, const struct msg *,
                                struct msg **);

void msg_dump(const struct msg *);

#endif
