#include "net.h"
#include "log.h"

#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define gai_print_errno(ec) print_error("getaddrinfo: %s\n", gai_strerror(ec))

static int ignore_signal(int signum)
{
	int ret = 0;
	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_handler = SIG_IGN;

	ret = sigaction(signum, &act, NULL);
	if (ret < 0) {
		print_errno("sigaction");
		return ret;
	}

	return ret;
}

static int ignore_sigchld()
{
	return ignore_signal(SIGCHLD);
}

int bind_to_port(const char *port)
{
	struct addrinfo *result, *it = NULL;
	struct addrinfo hints;
	int socket_fd, ret = 0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	ret = getaddrinfo(NULL, port, &hints, &result);
	if (ret) {
		gai_print_errno(ret);
		return -1;
	}

	for (it = result; it; it = it->ai_next) {
		socket_fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
		if (socket_fd < 0) {
			print_errno("socket");
			continue;
		}

		static const int yes = 1;

		if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
			print_errno("setsockopt");
			goto close_socket;
		}

		if (bind(socket_fd, it->ai_addr, it->ai_addrlen) < 0) {
			print_errno("bind");
			goto close_socket;
		}

		break;

	close_socket:
		close(socket_fd);
	}

	freeaddrinfo(result);

	if (!it) {
		print_error("Couldn't bind to port %s\n", port);
		return -1;
	}

	ret = listen(socket_fd, 4096);
	if (ret < 0) {
		print_errno("listen");
		goto fail;
	}

	/* Don't make zombies. The alternative is wait()ing for the child in
	 * the signal handler. */
	ret = ignore_sigchld();
	if (ret < 0)
		goto fail;

	return socket_fd;

fail:
	close(socket_fd);

	return ret;
}

int accept_connection(int fd, connection_handler handler, void *arg)
{
	int conn_fd, ret = 0;

	ret = accept(fd, NULL, NULL);
	if (ret < 0) {
		print_errno("accept");
		return ret;
	}
	conn_fd = ret;

	pid_t child_pid = fork();
	if (child_pid < 0) {
		print_errno("fork");
		ret = -1;
		goto close_conn;
	}

	if (!child_pid) {
		close(fd);
		ret = handler(conn_fd, arg);
		close(conn_fd);
		exit(ret);
	}

close_conn:
	close(conn_fd);

	return ret;
}

int connect_to_host(const char *host, const char *port)
{
	struct addrinfo *result, *it = NULL;
	struct addrinfo hints;
	int socket_fd, ret = 0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	ret = getaddrinfo(host, port, &hints, &result);
	if (ret) {
		gai_print_errno(ret);
		return -1;
	}

	for (it = result; it; it = it->ai_next) {
		socket_fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
		if (socket_fd < 0) {
			print_errno("socket");
			continue;
		}

		if (connect(socket_fd, it->ai_addr, it->ai_addrlen) < 0) {
			print_errno("connect");
			goto close_socket;
		}

		break;

	close_socket:
		close(socket_fd);
	}

	freeaddrinfo(result);

	if (!it) {
		print_error("Couldn't connect to host %s, port %s\n", host, port);
		return -1;
	}

	return socket_fd;
}

int send_all(int fd, const void *buf, size_t len)
{
	size_t sent_total = 0;

	while (sent_total < len) {
		ssize_t sent_now = write(fd, (const char *)buf + sent_total, len - sent_total);

		if (sent_now < 0) {
			print_errno("write");
			return -1;
		}

		sent_total += sent_now;
	}

	return 0;
}

int recv_all(int fd, void *buf, size_t len)
{
	size_t read_total = 0;

	while (read_total < len) {
		ssize_t read_now = read(fd, buf, len);

		if (read_now < 0) {
			print_errno("read");
			return -1;
		}

		read_total += read_now;
	}

	return read_total;
}

int send_buf(int fd, const void *buf, size_t len)
{
	int ret = 0;

	ret = send_all(fd, &len, sizeof(len));
	if (ret < 0)
		return ret;

	ret = send_all(fd, buf, len);
	if (ret < 0)
		return ret;

	return ret;
}

int recv_buf(int fd, void **buf, size_t *len)
{
	int ret = 0;

	ret = recv_all(fd, len, sizeof(*len));
	if (ret < 0)
		return ret;

	*buf = malloc(*len);
	if (!*buf) {
		print_errno("malloc");
		return -1;
	}

	ret = recv_all(fd, *buf, *len);
	if (ret < 0)
		goto free_buf;

	return ret;

free_buf:
	free(*buf);

	return ret;
}

int recv_static(int fd, void *buf, size_t len)
{
	void *actual_buf;
	size_t actual_len;
	int ret = 0;

	ret = recv_buf(fd, &actual_buf, &actual_len);
	if (ret < 0)
		return ret;

	if (actual_len != len) {
		print_error("Expected message length: %lu, actual: %lu\n", len, actual_len);
		ret = -1;
		goto free_buf;
	}

	memcpy(buf, actual_buf, len);

free_buf:
	free(actual_buf);

	return ret;
}
