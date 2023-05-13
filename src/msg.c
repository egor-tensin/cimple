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

struct msg {
	size_t argc;
	const char **argv;
};

size_t msg_get_length(const struct msg *msg)
{
	return msg->argc;
}

const char **msg_get_words(const struct msg *msg)
{
	return msg->argv;
}

int msg_success(struct msg **msg)
{
	const char *argv[] = {"success", NULL};
	return msg_from_argv(msg, argv);
}

int msg_error(struct msg **msg)
{
	const char *argv[] = {"error", NULL};
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
	struct msg *dest = NULL;
	int ret = 0;

	*_dest = malloc(sizeof(struct msg));
	if (!_dest) {
		log_errno("malloc");
		return -1;
	}
	dest = *_dest;

	dest->argc = src->argc;

	ret = msg_copy_argv(dest, (const char **)src->argv);
	if (ret < 0)
		goto free;

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
	struct msg *msg = NULL;
	int ret = 0;

	*_msg = malloc(sizeof(struct msg));
	if (!*_msg) {
		log_errno("malloc");
		return -1;
	}
	msg = *_msg;

	msg->argc = 0;
	for (const char **s = argv; *s; ++s)
		++msg->argc;

	ret = msg_copy_argv(msg, argv);
	if (ret < 0)
		goto free;

	return 0;

free:
	free(msg);

	return -1;
}

static uint32_t calc_buf_size(const struct msg *msg)
{
	uint32_t len = 0;
	for (size_t i = 0; i < msg->argc; ++i)
		len += strlen(msg->argv[i]) + 1;
	return len;
}

static size_t calc_argv_len(const void *buf, size_t len)
{
	size_t argc = 0;
	for (const char *it = buf; it < (const char *)buf + len; it += strlen(it) + 1)
		++argc;
	return argc;
}

static void argv_pack(char *dest, const struct msg *msg)
{
	for (size_t i = 0; i < msg->argc; ++i) {
		strcpy(dest, msg->argv[i]);
		dest += strlen(msg->argv[i]) + 1;
	}
}

static int argv_unpack(struct msg *msg, const char *src)
{
	size_t copied = 0;

	msg->argv = calloc(msg->argc + 1, sizeof(const char *));
	if (!msg->argv) {
		log_errno("calloc");
		return -1;
	}

	for (copied = 0; copied < msg->argc; ++copied) {
		msg->argv[copied] = strdup(src);
		if (!msg->argv[copied]) {
			log_errno("strdup");
			goto free;
		}

		src += strlen(msg->argv[copied]) + 1;
	}

	return 0;

free:
	for (size_t i = 0; i < copied; ++i) {
		free((char *)msg->argv[i]);
	}

	msg_free(msg);

	return -1;
}

int msg_send(int fd, const struct msg *msg)
{
	struct buf *buf;
	int ret = 0;

	uint32_t size = calc_buf_size(msg);
	char *data = malloc(size);
	if (!data) {
		log_errno("malloc");
		return -1;
	}
	argv_pack(data, msg);

	ret = buf_create(&buf, data, size);
	if (ret < 0)
		goto free_data;

	ret = net_send_buf(fd, buf);
	if (ret < 0)
		goto destroy_buf;

destroy_buf:
	buf_destroy(buf);

free_data:
	free(data);

	return ret;
}

int msg_send_and_wait(int fd, const struct msg *request, struct msg **response)
{
	int ret = 0;

	ret = msg_send(fd, request);
	if (ret < 0)
		return ret;

	ret = msg_recv(fd, response);
	if (ret < 0)
		return ret;

	return ret;
}

int msg_recv(int fd, struct msg **_msg)
{
	struct msg *msg = NULL;
	struct buf *buf = NULL;
	int ret = 0;

	ret = net_recv_buf(fd, &buf);
	if (ret < 0)
		return ret;

	*_msg = malloc(sizeof(struct msg));
	if (!*_msg) {
		log_errno("malloc");
		ret = -1;
		goto destroy_buf;
	}
	msg = *_msg;

	msg->argc = calc_argv_len(buf_get_data(buf), buf_get_size(buf));

	ret = argv_unpack(msg, buf_get_data(buf));
	if (ret < 0)
		goto free_msg;

	goto destroy_buf;

free_msg:
	free(msg);

destroy_buf:
	buf_destroy(buf);

	return ret;
}

void msg_dump(const struct msg *msg)
{
	log("Message[%zu]:\n", msg->argc);
	for (size_t i = 0; i < msg->argc; ++i)
		log("\t%s\n", msg->argv[i]);
}
