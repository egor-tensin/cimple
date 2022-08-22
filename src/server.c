#include "server.h"
#include "cmd.h"
#include "net.h"

#include <stdio.h>
#include <unistd.h>

int server_create(struct server *server, const struct settings *settings)
{
	server->fd = bind_to_port(settings->port);
	if (server->fd < 0)
		return server->fd;

	return 0;
}

void server_destroy(const struct server *server)
{
	close(server->fd);
}

static int cmd_handle(const struct cmd *cmd, void *)
{
	for (int i = 0; i < cmd->argc; ++i)
		printf("cmd[%d]: %s\n", i, cmd->argv[i]);
	return 0;
}

static int server_handle(int fd, void *)
{
	return cmd_recv_and_send_result(fd, cmd_handle, NULL);
}

static int server_accept(const struct server *server)
{
	return accept_connection(server->fd, server_handle, NULL);
}

int server_main(const struct server *server)
{
	int ret = 0;

	while (1) {
		ret = server_accept(server);
		if (ret < 0)
			return ret;
	}
}
