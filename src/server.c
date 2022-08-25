#include "server.h"
#include "msg.h"
#include "net.h"
#include "tcp_server.h"

#include <stdio.h>
#include <unistd.h>

int server_create(struct server *server, const struct settings *settings)
{
	int ret = 0;

	ret = tcp_server_create(&server->tcp_server, settings->port);
	if (ret < 0)
		return ret;

	return 0;
}

void server_destroy(const struct server *server)
{
	tcp_server_destroy(&server->tcp_server);
}

static int msg_handle(const struct msg *msg, void *)
{
	return msg_dump_unknown(msg);
}

static int server_conn_handler(int fd, void *)
{
	return msg_recv_and_send_result(fd, msg_handle, NULL);
}

int server_main(const struct server *server)
{
	int ret = 0;

	while (1) {
		ret = tcp_server_accept(&server->tcp_server, server_conn_handler, NULL);
		if (ret < 0)
			return ret;
	}
}
