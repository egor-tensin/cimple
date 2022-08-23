#include "cmd.h"
#include "log.h"
#include "net.h"

#include <stdlib.h>
#include <string.h>

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
	for (int i = 0; i < argc; ++i)
		argv[i] = NULL;

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

int cmd_send(int fd, const struct cmd *cmd)
{
	int ret = 0;

	size_t len = calc_buf_len(cmd->argc, cmd->argv);
	char *buf = malloc(len);
	if (!buf) {
		print_errno("malloc");
		return -1;
	}
	arr_pack(buf, cmd->argc, cmd->argv);

	ret = send_buf(fd, buf, len);
	if (ret < 0)
		goto free_buf;

free_buf:
	free(buf);

	return ret;
}

int cmd_send_and_wait_for_result(int fd, const struct cmd *cmd, int *result)
{
	int ret = 0;

	ret = cmd_send(fd, cmd);
	if (ret < 0)
		return ret;

	ret = recv_static(fd, result, sizeof(*result));
	if (ret < 0)
		return ret;

	return ret;
}

int cmd_recv(int fd, struct cmd *cmd)
{
	void *buf;
	size_t len;
	int ret = 0;

	ret = recv_buf(fd, &buf, &len);
	if (ret < 0)
		return ret;

	cmd->argc = calc_arr_len(buf, len);
	cmd->argv = malloc(cmd->argc * sizeof(char *));
	if (!cmd->argv) {
		print_errno("malloc");
		ret = -1;
		goto free_buf;
	}
	memset(cmd->argv, 0, cmd->argc * sizeof(char *));

	ret = arr_unpack(cmd->argv, cmd->argc, buf);
	if (ret < 0)
		goto free_argv;

	goto free_buf;

free_argv:
	free(cmd->argv);

free_buf:
	free(buf);

	return ret;
}

int cmd_recv_and_send_result(int fd, cmd_handler handler, void *arg)
{
	struct cmd cmd;
	int result;
	int ret = 0;

	ret = cmd_recv(fd, &cmd);
	if (ret < 0)
		return ret;

	result = handler(&cmd, arg);

	ret = send_buf(fd, &result, sizeof(result));
	if (ret < 0)
		goto free_cmd;

free_cmd:
	cmd_free(&cmd);

	return ret;
}

void cmd_free(const struct cmd *cmd)
{
	for (int i = 0; i < cmd->argc; ++i)
		free(cmd->argv[i]);
	free(cmd->argv);
}