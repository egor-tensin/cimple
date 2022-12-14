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

int msg_success(struct msg *msg)
{
	char *argv[] = {"success", NULL};
	return msg_from_argv(msg, argv);
}

int msg_error(struct msg *msg)
{
	char *argv[] = {"error", NULL};
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

static int msg_copy_argv(struct msg *msg, char **argv)
{
	msg->argv = calloc(msg->argc, sizeof(char *));

	if (!msg->argv) {
		log_errno("calloc");
		return -1;
	}

	for (int i = 0; i < msg->argc; ++i) {
		msg->argv[i] = strdup(argv[i]);
		if (!msg->argv[i]) {
			log_errno("strdup");
			goto free;
		}
	}

	return 0;

free:
	msg_free(msg);

	return -1;
}

struct msg *msg_copy(const struct msg *src)
{
	struct msg *dest;
	int ret = 0;

	dest = malloc(sizeof(*dest));
	if (!dest) {
		log_errno("calloc");
		return NULL;
	}
	dest->argc = src->argc;

	ret = msg_copy_argv(dest, src->argv);
	if (ret < 0)
		goto free;

	return dest;

free:
	free(dest);

	return NULL;
}

void msg_free(const struct msg *msg)
{
	for (int i = 0; i < msg->argc; ++i)
		free((char *)msg->argv[i]);
	free(msg->argv);
}

int msg_from_argv(struct msg *msg, char **argv)
{
	int argc = 0;

	for (char **s = argv; *s; ++s)
		++argc;

	msg->argc = argc;
	return msg_copy_argv(msg, argv);
}

static uint32_t calc_buf_len(const struct msg *msg)
{
	uint32_t len = 0;
	for (int i = 0; i < msg->argc; ++i)
		len += strlen(msg->argv[i]) + 1;
	return len;
}

static int calc_argv_len(const void *buf, size_t len)
{
	int argc = 0;
	for (const char *it = buf; it < (const char *)buf + len; it += strlen(it) + 1)
		++argc;
	return argc;
}

static void argv_pack(char *dest, const struct msg *msg)
{
	for (int i = 0; i < msg->argc; ++i) {
		strcpy(dest, msg->argv[i]);
		dest += strlen(msg->argv[i]) + 1;
	}
}

static int argv_unpack(struct msg *msg, const char *src)
{
	msg->argv = calloc(msg->argc, sizeof(char *));
	if (!msg->argv) {
		log_errno("calloc");
		return -1;
	}

	for (int i = 0; i < msg->argc; ++i) {
		size_t len = strlen(src);

		msg->argv[i] = malloc(len + 1);
		if (!msg->argv[i]) {
			log_errno("malloc");
			goto free;
		}

		strcpy(msg->argv[i], src);
		src += len + 1;
	}

	return 0;

free:
	msg_free(msg);

	return -1;
}

int msg_send(int fd, const struct msg *msg)
{
	int ret = 0;

	uint32_t len = calc_buf_len(msg);
	char *buf = malloc(len);
	if (!buf) {
		log_errno("malloc");
		return -1;
	}
	argv_pack(buf, msg);

	ret = net_send_buf(fd, buf, len);
	if (ret < 0)
		goto free_buf;

free_buf:
	free(buf);

	return ret;
}

int msg_send_and_wait(int fd, const struct msg *request, struct msg *response)
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

int msg_recv(int fd, struct msg *msg)
{
	void *buf;
	uint32_t len;
	int ret = 0;

	ret = net_recv_buf(fd, &buf, &len);
	if (ret < 0)
		return ret;

	msg->argc = calc_argv_len(buf, len);

	ret = argv_unpack(msg, buf);
	if (ret < 0)
		goto free_buf;

free_buf:
	free(buf);

	return ret;
}

void msg_dump(const struct msg *msg)
{
	log("Message[%d]:\n", msg->argc);
	for (int i = 0; i < msg->argc; ++i)
		log("\t%s\n", msg->argv[i]);
}
