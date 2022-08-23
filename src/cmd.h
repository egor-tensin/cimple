#ifndef __CMD_H__
#define __CMD_H__

struct cmd {
	int argc;
	char **argv;
};

int cmd_from_argv(struct cmd *, const char *argv[]);

int cmd_send(int fd, const struct cmd *);
int cmd_send_and_wait_for_result(int fd, const struct cmd *, int *result);

typedef int (*cmd_handler)(const struct cmd *, void *arg);

int cmd_recv(int fd, struct cmd *);
int cmd_recv_and_send_result(int fd, cmd_handler, void *arg);

void cmd_free(const struct cmd *);

#endif
