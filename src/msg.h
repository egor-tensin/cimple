/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __MSG_H__
#define __MSG_H__

struct msg {
	int argc;
	char **argv;
};

int msg_success(struct msg *);
int msg_error(struct msg *);

int msg_is_success(const struct msg *);
int msg_is_error(const struct msg *);

struct msg *msg_copy(const struct msg *);
void msg_free(const struct msg *);

int msg_from_argv(struct msg *, char **argv);

int msg_recv(int fd, struct msg *);
int msg_send(int fd, const struct msg *);

int msg_send_and_wait(int fd, const struct msg *, struct msg *response);

void msg_dump(const struct msg *);

#endif
