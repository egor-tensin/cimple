#include "signal.h"
#include "log.h"

#include <signal.h>

volatile sig_atomic_t global_stop_flag = 0;

int signal_set(const sigset_t *new, sigset_t *old)
{
	int ret = 0;

	ret = sigprocmask(SIG_SETMASK, new, old);
	if (ret < 0) {
		print_errno("sigprocmask");
		return ret;
	}

	return ret;
}

int signal_block_parent(sigset_t *old)
{
	sigset_t new;
	sigfillset(&new);
	return signal_set(&new, old);
}

int signal_block_child()
{
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGQUIT);
	sigaddset(&set, SIGTERM);
	return signal_set(&set, NULL);
}
