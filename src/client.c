#include "client.h"
#include "msg.h"
#include "net.h"

#include <unistd.h>

int client_create(struct client *client, const struct settings *settings)
{
	client->fd = connect_to_host(settings->host, settings->port);
	if (client->fd < 0)
		return client->fd;

	return 0;
}

void client_destroy(const struct client *client)
{
	close(client->fd);
}

int client_main(const struct client *client, int argc, char *argv[])
{
	int result, ret = 0;
	struct msg msg = {argc, argv};

	ret = msg_send_and_wait_for_result(client->fd, &msg, &result);
	if (ret < 0)
		return ret;

	return result;
}
