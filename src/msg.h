#ifndef __MSG_H__
#define __MSG_H__

struct msg {
	int argc;
	char **argv;
};

struct msg *msg_copy(const struct msg *);
void msg_free(const struct msg *);

int msg_from_argv(struct msg *, char **argv);

int msg_recv(int fd, struct msg *);
int msg_send(int fd, const struct msg *);

typedef int (*msg_handler)(const struct msg *, void *arg);
int msg_send_and_wait(int fd, const struct msg *, int *result);
int msg_recv_and_handle(int fd, msg_handler, void *arg);

int msg_dump_unknown(const struct msg *);

#endif
