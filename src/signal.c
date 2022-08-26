#include "signal.h"
#include "log.h"

#include <pthread.h>
#include <signal.h>

volatile sig_atomic_t global_stop_flag = 0;

int signal_set_thread_attr(pthread_attr_t *attr)
{
	int ret = 0;

	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGQUIT);
	sigaddset(&set, SIGTERM);

	ret = pthread_attr_setsigmask_np(attr, &set);
	if (ret) {
		pthread_print_errno(ret, "pthread_attr_setsigmask_np");
		return ret;
	}

	return ret;
}
