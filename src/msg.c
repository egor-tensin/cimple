/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "msg.h"
#include "log.h"
#include "net.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct msg {
	size_t argc;
	const char **argv;
};

size_t msg_get_length(const struct msg *msg)
{
	return msg->argc;
}

const char **msg_get_strings(const struct msg *msg)
{
	return msg->argv;
}

const char *msg_get_first_string(const struct msg *msg)
{
	return msg->argv[0];
}

int msg_success(struct msg **msg)
{
	static const char *argv[] = {"success", NULL};
	return msg_from_argv(msg, argv);
}

int msg_error(struct msg **msg)
{
	static const char *argv[] = {"error", NULL};
	return msg_from_argv(msg, argv);
}

int msg_is_success(const struct msg *msg)
{
	return msg->argc == 1 && !strcmp(msg->argv[0], "success");
}

int msg_is_error(const struct msg *msg)
{
	return msg->argc == 1 && !strcmp(msg->argv[0], "error");
}

static int msg_copy_argv(struct msg *msg, const char **argv)
{
	size_t copied = 0;

	msg->argv = calloc(msg->argc + 1, sizeof(const char *));
	if (!msg->argv) {
		log_errno("calloc");
		return -1;
	}

	for (copied = 0; copied < msg->argc; ++copied) {
		msg->argv[copied] = strdup(argv[copied]);
		if (!msg->argv[copied]) {
			log_errno("strdup");
			goto free_copied;
		}
	}

	return 0;

free_copied:
	for (size_t i = 0; i < copied; ++i) {
		free((char *)msg->argv[i]);
	}

	free(msg->argv);

	return -1;
}

int msg_copy(struct msg **_dest, const struct msg *src)
{
	int ret = 0;

	struct msg *dest = malloc(sizeof(struct msg));
	if (!dest) {
		log_errno("malloc");
		return -1;
	}

	dest->argc = src->argc;

	ret = msg_copy_argv(dest, (const char **)src->argv);
	if (ret < 0)
		goto free;

	*_dest = dest;
	return 0;

free:
	free(dest);

	return -1;
}

void msg_free(struct msg *msg)
{
	for (size_t i = 0; i < msg->argc; ++i)
		free((char *)msg->argv[i]);
	free(msg->argv);
	free(msg);
}

int msg_from_argv(struct msg **_msg, const char **argv)
{
	int ret = 0;

	struct msg *msg = malloc(sizeof(struct msg));
	if (!msg) {
		log_errno("malloc");
		return -1;
	}

	msg->argc = 0;
	for (const char **s = argv; *s; ++s)
		++msg->argc;

	if (!msg->argc) {
		log_err("A message must contain at least one string\n");
		goto free;
	}

	ret = msg_copy_argv(msg, argv);
	if (ret < 0)
		goto free;

	*_msg = msg;
	return 0;

free:
	free(msg);

	return -1;
}

int msg_send(int fd, const struct msg *msg)
{
	struct buf *buf = NULL;
	int ret = 0;

	ret = buf_pack_strings(&buf, msg->argc, msg->argv);
	if (ret < 0)
		return ret;

	ret = net_send_buf(fd, buf);
	if (ret < 0)
		goto destroy_buf;

destroy_buf:
	buf_destroy(buf);

	return ret;
}

int msg_recv(int fd, struct msg **_msg)
{
	struct buf *buf = NULL;
	int ret = 0;

	ret = net_recv_buf(fd, &buf);
	if (ret < 0)
		return ret;

	struct msg *msg = malloc(sizeof(struct msg));
	if (!msg) {
		log_errno("malloc");
		ret = -1;
		goto destroy_buf;
	}

	ret = buf_unpack_strings(buf, &msg->argc, &msg->argv);
	if (ret < 0)
		goto free_msg;

	if (!msg->argc) {
		log_err("A message must contain at least one string\n");
		goto free_msg;
	}

	*_msg = msg;
	goto destroy_buf;

free_msg:
	free(msg);

destroy_buf:
	buf_destroy(buf);

	return ret;
}

int msg_communicate(int fd, const struct msg *request, struct msg **response)
{
	int ret = 0;

	if (!request && !response) {
		log_err("For communication, there must be at least a request and/or response\n");
		return -1;
	}

	if (request) {
		ret = msg_send(fd, request);
		if (ret < 0)
			return ret;
	}

	if (response) {
		ret = msg_recv(fd, response);
		if (ret < 0)
			return ret;
	}

	return ret;
}

int msg_connect_and_communicate(const char *host, const char *port, const struct msg *request,
                                struct msg **response)
{
	int fd = -1, ret = 0;

	fd = net_connect(host, port);
	if (fd < 0)
		return fd;

	ret = msg_communicate(fd, request, response);
	if (ret < 0)
		goto close;

close:
	log_errno_if(close(fd), "close");

	return ret;
}

void msg_dump(const struct msg *msg)
{
	log("Message[%zu]:\n", msg->argc);
	for (size_t i = 0; i < msg->argc; ++i)
		log("\t%s\n", msg->argv[i]);
}
