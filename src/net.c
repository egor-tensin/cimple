/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "net.h"
#include "file.h"
#include "log.h"

#include <netdb.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define gai_log_errno(ec) log_err("getaddrinfo: %s\n", gai_strerror(ec))

int net_bind(const char *port)
{
	static const int flags = SOCK_CLOEXEC;
	struct addrinfo *result = NULL, *it = NULL;
	struct addrinfo hints;
	int socket_fd = -1, ret = 0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	ret = getaddrinfo(NULL, port, &hints, &result);
	if (ret) {
		gai_log_errno(ret);
		return -1;
	}

	for (it = result; it; it = it->ai_next) {
		socket_fd = socket(it->ai_family, it->ai_socktype | flags, it->ai_protocol);
		if (socket_fd < 0) {
			log_errno("socket");
			continue;
		}

		static const int yes = 1;
		static const int no = 0;

		if (it->ai_family == AF_INET6) {
			if (setsockopt(socket_fd, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no)) < 0) {
				log_errno("setsockopt");
				goto close_socket;
			}
		}

		if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
			log_errno("setsockopt");
			goto close_socket;
		}

		if (bind(socket_fd, it->ai_addr, it->ai_addrlen) < 0) {
			log_errno("bind");
			goto close_socket;
		}

		break;

	close_socket:
		net_close(socket_fd);
	}

	freeaddrinfo(result);

	if (!it) {
		log_err("Couldn't bind to port %s\n", port);
		return -1;
	}

	ret = listen(socket_fd, 4096);
	if (ret < 0) {
		log_errno("listen");
		goto fail;
	}

	return socket_fd;

fail:
	net_close(socket_fd);

	return ret;
}

int net_accept(int fd)
{
	static const int flags = SOCK_CLOEXEC;
	int ret = 0;

	ret = accept4(fd, NULL, NULL, flags);
	if (ret < 0) {
		log_errno("accept4");
		return ret;
	}

	return ret;
}

int net_connect(const char *host, const char *port)
{
	static const int flags = SOCK_CLOEXEC;
	struct addrinfo *result = NULL, *it = NULL;
	struct addrinfo hints;
	int socket_fd = -1, ret = 0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	ret = getaddrinfo(host, port, &hints, &result);
	if (ret) {
		gai_log_errno(ret);
		return -1;
	}

	for (it = result; it; it = it->ai_next) {
		socket_fd = socket(it->ai_family, it->ai_socktype | flags, it->ai_protocol);
		if (socket_fd < 0) {
			log_errno("socket");
			continue;
		}

		if (connect(socket_fd, it->ai_addr, it->ai_addrlen) < 0) {
			log_errno("connect");
			goto close_socket;
		}

		break;

	close_socket:
		net_close(socket_fd);
	}

	freeaddrinfo(result);

	if (!it) {
		log_err("Couldn't connect to host %s, port %s\n", host, port);
		return -1;
	}

	return socket_fd;
}

void net_close(int fd)
{
	file_close(fd);
}

static ssize_t net_send_part(int fd, const void *buf, size_t size)
{
	static const int flags = MSG_NOSIGNAL;

	ssize_t ret = send(fd, buf, size, flags);
	if (ret < 0) {
		log_errno("send");
		return -1;
	}

	return ret;
}

int net_send(int fd, const void *buf, size_t size)
{
	size_t sent_total = 0;

	while (sent_total < size) {
		ssize_t sent_now =
		    net_send_part(fd, (const char *)buf + sent_total, size - sent_total);
		if (sent_now < 0)
			return -1;
		sent_total += sent_now;
	}

	return 0;
}

int net_recv(int fd, void *buf, size_t size)
{
	ssize_t read_total = 0;

	while ((size_t)read_total < size) {
		ssize_t read_now = read(fd, (unsigned char *)buf + read_total, size - read_total);
		if (!read_now)
			break;

		if (read_now < 0) {
			log_errno("read");
			return -1;
		}

		read_total += read_now;
	}

	if ((size_t)read_total < size) {
		log_err("Received only %zd bytes out of %zu\n", read_total, size);
		return -1;
	}

	return 0;
}

struct buf {
	uint32_t size;
	void *data;
};

int buf_create(struct buf **_buf, const void *data, uint32_t size)
{
	int ret = 0;

	struct buf *buf = malloc(sizeof(struct buf));
	if (!buf) {
		log_errno("malloc");
		return -1;
	}

	buf->data = malloc(size);
	if (!buf->data) {
		log_errno("malloc");
		goto free;
	}

	buf->size = size;
	memcpy(buf->data, data, size);

	*_buf = buf;
	return ret;

free:
	free(buf);

	return ret;
}

void buf_destroy(struct buf *buf)
{
	free(buf->data);
	free(buf);
}

uint32_t buf_get_size(const struct buf *buf)
{
	return buf->size;
}

void *buf_get_data(const struct buf *buf)
{
	return buf->data;
}

static size_t count_strings(const void *_data, size_t size)
{
	const unsigned char *data = (const unsigned char *)_data;
	const unsigned char *it = memchr(data, '\0', size);

	size_t numof_strings = 0;
	while (it) {
		it = memchr(it + 1, '\0', size - (it - data) - 1);
		++numof_strings;
	}

	return numof_strings;
}

int buf_pack_strings(struct buf **_buf, size_t argc, const char **argv)
{
	struct buf *buf = malloc(sizeof(struct buf));
	if (!buf) {
		log_errno("malloc");
		return -1;
	}

	buf->size = 0;
	for (size_t i = 0; i < argc; ++i)
		buf->size += strlen(argv[i]) + 1;

	buf->data = malloc(buf->size);
	if (!buf->data) {
		log_errno("malloc");
		goto free_buf;
	}

	char *it = (char *)buf->data;
	for (size_t i = 0; i < argc; ++i) {
		it = stpcpy(it, argv[i]) + 1;
	}

	*_buf = buf;
	return 0;

free_buf:
	free(buf);

	return -1;
}

int buf_unpack_strings(const struct buf *buf, size_t *_argc, const char ***_argv)
{
	size_t argc = count_strings(buf->data, buf->size);
	size_t copied = 0;

	const char **argv = calloc(argc + 1, sizeof(const char *));
	if (!argv) {
		log_errno("calloc");
		return -1;
	}

	const char *it = (const char *)buf->data;
	for (copied = 0; copied < argc; ++copied) {
		argv[copied] = strdup(it);
		if (!argv[copied]) {
			log_errno("strdup");
			goto free;
		}

		it += strlen(argv[copied]) + 1;
	}

	*_argc = argc;
	*_argv = argv;
	return 0;

free:
	for (size_t i = 0; i < copied; ++i) {
		free((char *)argv[i]);
	}

	free(argv);

	return -1;
}

int net_send_buf(int fd, const struct buf *buf)
{
	int ret = 0;

	uint32_t size = htonl(buf->size);
	ret = net_send(fd, &size, sizeof(size));
	if (ret < 0)
		return ret;

	ret = net_send(fd, buf->data, buf->size);
	if (ret < 0)
		return ret;

	return ret;
}

int net_recv_buf(int fd, struct buf **buf)
{
	uint32_t size = 0;
	int ret = 0;

	ret = net_recv(fd, &size, sizeof(size));
	if (ret < 0) {
		log_err("Couldn't read buffer size\n");
		goto fail;
	}
	size = ntohl(size);

	void *data = malloc(size);
	if (!data) {
		log_errno("malloc");
		goto fail;
	}

	ret = net_recv(fd, data, size);
	if (ret < 0) {
		log_err("Couldn't read buffer\n");
		goto free_data;
	}

	ret = buf_create(buf, data, size);
	if (ret < 0)
		goto free_data;

	free(data);

	return ret;

free_data:
	free(data);

fail:
	return -1;
}
