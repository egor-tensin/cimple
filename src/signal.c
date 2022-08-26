#include "signal.h"
#include "log.h"

#include <pthread.h>
#include <signal.h>

volatile sig_atomic_t global_stop_flag = 0;

int signal_set_thread_attr(pthread_attr_t *attr)
{
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGQUIT);
	sigaddset(&set, SIGTERM);

	if (pthread_attr_setsigmask_np(attr, &set)) {
		print_errno("pthread_attr_setsigmask_np");
		return -1;
	}

	return 0;
}
