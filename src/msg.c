#include "msg.h"
#include "log.h"
#include "net.h"

#include <stdlib.h>
#include <string.h>

void msg_free(const struct msg *msg)
{
	for (int i = 0; i < msg->argc; ++i)
		free(msg->argv[i]);
	free(msg->argv);
}

int msg_from_argv(struct msg *msg, const char *argv[])
{
	int argc = 0;

	for (const char **s = argv; *s; ++s)
		++argc;

	msg->argc = argc;
	msg->argv = calloc(argc, sizeof(char *));

	if (!msg->argv) {
		print_errno("calloc");
		return -1;
	}

	for (int i = 0; i < argc; ++i) {
		msg->argv[i] = strdup(argv[i]);
		if (!msg->argv[i]) {
			print_errno("strdup");
			goto free;
		}
	}

	return 0;

free:
	for (int i = 0; i < argc; ++i)
		if (msg->argv[i])
			free(msg->argv[i]);
		else
			break;

	free(msg->argv);
	return -1;
}

static size_t calc_buf_len(int argc, char **argv)
{
	size_t len = 0;
	for (int i = 0; i < argc; ++i)
		len += strlen(argv[i]) + 1;
	return len;
}

static int calc_arr_len(const void *buf, size_t len)
{
	int argc = 0;
	for (const char *it = buf; it < (const char *)buf + len; it += strlen(it) + 1)
		++argc;
	return argc;
}

static void arr_pack(char *dest, int argc, char **argv)
{
	for (int i = 0; i < argc; ++i) {
		strcpy(dest, argv[i]);
		dest += strlen(argv[i]) + 1;
	}
}

static int arr_unpack(char **argv, int argc, const char *src)
{
	for (int i = 0; i < argc; ++i) {
		size_t len = strlen(src);

		argv[i] = malloc(len);
		if (!argv[i]) {
			print_errno("malloc");
			goto free;
		}

		strcpy(argv[i], src);
		src += len + 1;
	}

	return 0;

free:
	for (int i = 0; i < argc; ++i)
		if (argv[i])
			free(argv[i]);
		else
			break;

	return -1;
}

int msg_send(int fd, const struct msg *msg)
{
	int ret = 0;

	size_t len = calc_buf_len(msg->argc, msg->argv);
	char *buf = malloc(len);
	if (!buf) {
		print_errno("malloc");
		return -1;
	}
	arr_pack(buf, msg->argc, msg->argv);

	ret = net_send_buf(fd, buf, len);
	if (ret < 0)
		goto free_buf;

free_buf:
	free(buf);

	return ret;
}

int msg_send_and_wait(int fd, const struct msg *msg, int *result)
{
	int ret = 0;

	ret = msg_send(fd, msg);
	if (ret < 0)
		return ret;

	ret = net_recv_static(fd, result, sizeof(*result));
	if (ret < 0)
		return ret;

	return ret;
}

int msg_recv(int fd, struct msg *msg)
{
	void *buf;
	size_t len;
	int ret = 0;

	ret = net_recv_buf(fd, &buf, &len);
	if (ret < 0)
		return ret;

	msg->argc = calc_arr_len(buf, len);
	msg->argv = calloc(msg->argc, sizeof(char *));
	if (!msg->argv) {
		print_errno("calloc");
		ret = -1;
		goto free_buf;
	}

	ret = arr_unpack(msg->argv, msg->argc, buf);
	if (ret < 0)
		goto free_argv;

	goto free_buf;

free_argv:
	free(msg->argv);

free_buf:
	free(buf);

	return ret;
}

int msg_recv_and_handle(int fd, msg_handler handler, void *arg)
{
	struct msg msg;
	int result;
	int ret = 0;

	ret = msg_recv(fd, &msg);
	if (ret < 0)
		return ret;

	result = handler(&msg, arg);

	ret = net_send_buf(fd, &result, sizeof(result));
	if (ret < 0)
		goto free_msg;

free_msg:
	msg_free(&msg);

	return ret;
}

int msg_dump_unknown(const struct msg *msg)
{
	print_log("Received an unknown message:\n");
	for (int i = 0; i < msg->argc; ++i)
		print_log("\t%s\n", msg->argv[i]);
	return -1;
}
