#ifndef __SIGNAL_H__
#define __SIGNAL_H__

#include "compiler.h"

#include <pthread.h>
#include <signal.h>
#include <string.h>

extern volatile sig_atomic_t global_stop_flag;

static void signal_handler(UNUSED int signum)
{
	global_stop_flag = 1;
}

static __attribute__((constructor)) void signal_handler_install()
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signal_handler;

	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
}

int signal_set_thread_attr(pthread_attr_t *attr);

#endif
