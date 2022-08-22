#ifndef __SERVER_H__
#define __SERVER_H__

struct settings {
	const char *port;
};

struct server {
	int fd;
};

int server_create(struct server *, const struct settings *);
void server_destroy(const struct server *);

int server_main(const struct server *);

#endif
