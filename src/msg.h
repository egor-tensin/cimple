#ifndef __MSG_H__
#define __MSG_H__

struct msg {
	int argc;
	char **argv;
};

int msg_from_argv(struct msg *, const char *argv[]);

int msg_send(int fd, const struct msg *);
int msg_send_and_wait_for_result(int fd, const struct msg *, int *result);

typedef int (*msg_handler)(const struct msg *, void *arg);

int msg_recv(int fd, struct msg *);
int msg_recv_and_send_result(int fd, msg_handler, void *arg);

void msg_free(const struct msg *);

#endif
