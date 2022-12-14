/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "net.h"
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
	struct addrinfo *result, *it = NULL;
	struct addrinfo hints;
	int socket_fd, ret = 0;

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
		socket_fd = socket(it->ai_family, it->ai_socktype | SOCK_CLOEXEC, it->ai_protocol);
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
		log_errno_if(close(socket_fd), "close");
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
	log_errno_if(close(socket_fd), "close");

	return ret;
}

int net_accept(int fd)
{
	int ret = 0;

	ret = accept4(fd, NULL, NULL, SOCK_CLOEXEC);
	if (ret < 0) {
		log_errno("accept");
		return ret;
	}

	return ret;
}

int net_connect(const char *host, const char *port)
{
	struct addrinfo *result, *it = NULL;
	struct addrinfo hints;
	int socket_fd, ret = 0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	ret = getaddrinfo(host, port, &hints, &result);
	if (ret) {
		gai_log_errno(ret);
		return -1;
	}

	for (it = result; it; it = it->ai_next) {
		socket_fd = socket(it->ai_family, it->ai_socktype | SOCK_CLOEXEC, it->ai_protocol);
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
		log_errno_if(close(socket_fd), "close");
	}

	freeaddrinfo(result);

	if (!it) {
		log_err("Couldn't connect to host %s, port %s\n", host, port);
		return -1;
	}

	return socket_fd;
}

static ssize_t net_send(int fd, const void *buf, size_t len)
{
	static const int flags = MSG_NOSIGNAL;

	ssize_t ret = send(fd, buf, len, flags);
	if (ret < 0) {
		log_errno("send");
		return -1;
	}

	return ret;
}

int net_send_all(int fd, const void *buf, size_t len)
{
	size_t sent_total = 0;

	while (sent_total < len) {
		ssize_t sent_now = net_send(fd, (const char *)buf + sent_total, len - sent_total);
		if (sent_now < 0)
			return -1;
		sent_total += sent_now;
	}

	return 0;
}

int net_recv_all(int fd, void *buf, size_t len)
{
	ssize_t read_total = 0;

	while ((size_t)read_total < len) {
		ssize_t read_now = read(fd, buf, len);
		if (!read_now)
			break;

		if (read_now < 0) {
			log_errno("read");
			return -1;
		}

		read_total += read_now;
	}

	if ((size_t)read_total < len) {
		log_err("Received only %zd bytes out of %zu\n", read_total, len);
		return -1;
	}

	return 0;
}

int net_send_buf(int fd, const void *buf, uint32_t len)
{
	int ret = 0;

	len = htonl(len);
	ret = net_send_all(fd, &len, sizeof(len));
	if (ret < 0)
		return ret;
	len = ntohl(len);

	ret = net_send_all(fd, buf, len);
	if (ret < 0)
		return ret;

	return ret;
}

int net_recv_buf(int fd, void **buf, uint32_t *len)
{
	int ret = 0;

	ret = net_recv_all(fd, len, sizeof(*len));
	if (ret < 0) {
		log_err("Couldn't read buffer length\n");
		goto fail;
	}

	*len = ntohl(*len);

	*buf = malloc(*len);
	if (!*buf) {
		log_errno("malloc");
		goto fail;
	}

	ret = net_recv_all(fd, *buf, *len);
	if (ret < 0) {
		log_err("Couldn't read buffer\n");
		goto free_buf;
	}

	return 0;

free_buf:
	free(*buf);

fail:
	return -1;
}
