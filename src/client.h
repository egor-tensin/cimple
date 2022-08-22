#ifndef __CLIENT_H__
#define __CLIENT_H__

struct settings {
	const char *host;
	const char *port;
};

struct client {
	int fd;
};

int client_create(struct client *, const struct settings *);
void client_destroy(const struct client *);

int client_main(const struct client *, int argc, char *argv[]);

#endif
