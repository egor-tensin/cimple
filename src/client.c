#include "client.h"
#include "log.h"
#include "msg.h"
#include "net.h"

#include <unistd.h>

int client_create(struct client *client, const struct settings *settings)
{
	client->fd = net_connect(settings->host, settings->port);
	if (client->fd < 0)
		return client->fd;

	return 0;
}

void client_destroy(const struct client *client)
{
	check_errno(close(client->fd), "close");
}

int client_main(const struct client *client, int argc, char *argv[])
{
	struct msg request = {argc, argv};
	struct msg response;
	int ret = 0;

	ret = msg_send_and_wait(client->fd, &request, &response);
	if (ret < 0)
		return ret;

	if (msg_is_error(&response)) {
		print_error("Server failed to process the request\n");
		ret = -1;
		goto free_response;
	}

free_response:
	msg_free(&response);

	return ret;
}
