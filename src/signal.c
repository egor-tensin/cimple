/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "signal.h"
#include "compiler.h"
#include "log.h"

#include <signal.h>
#include <string.h>

volatile sig_atomic_t global_stop_flag = 0;

static void set_global_stop_flag(UNUSED int signum)
{
	global_stop_flag = 1;
}

static int my_sigaction(int signo, const struct sigaction *act)
{
	int ret = 0;

	ret = sigaction(signo, act, NULL);
	if (ret < 0) {
		log_errno("sigaction");
		return ret;
	}

	return ret;
}

int signal_install_global_handler()
{
	int ret = 0;

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = set_global_stop_flag;

	/* Don't care about proper cleanup here; we exit the program if this
	 * fails anyway. */
	ret = my_sigaction(SIGINT, &sa);
	if (ret < 0)
		return ret;
	ret = my_sigaction(SIGQUIT, &sa);
	if (ret < 0)
		return ret;
	ret = my_sigaction(SIGTERM, &sa);
	if (ret < 0)
		return ret;

	return ret;
}

int signal_set(const sigset_t *new, sigset_t *old)
{
	int ret = 0;

	ret = sigprocmask(SIG_SETMASK, new, old);
	if (ret < 0) {
		log_errno("sigprocmask");
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
