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
#include <stddef.h>
#include <string.h>

static int stop_signals[] = {SIGINT, SIGTERM, SIGQUIT};

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

int signal_handle_stop_signals(void)
{
	int ret = 0;

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = set_global_stop_flag;

	/* Don't care about proper cleanup here; we exit the program if this
	 * fails anyway. */

	for (size_t i = 0; i < sizeof(stop_signals) / sizeof(stop_signals[0]); ++i) {
		ret = my_sigaction(stop_signals[i], &sa);
		if (ret < 0)
			return ret;
	}

	return ret;
}

static int signal_set(const sigset_t *new, sigset_t *old)
{
	int ret = 0;

	ret = sigprocmask(SIG_SETMASK, new, old);
	if (ret < 0) {
		log_errno("sigprocmask");
		return ret;
	}

	return ret;
}

int signal_block_all(sigset_t *old)
{
	sigset_t new;
	sigfillset(&new);
	return signal_set(&new, old);
}

int signal_restore(const sigset_t *new)
{
	return signal_set(new, NULL);
}

int signal_unblock_all_except_stop_signals(void)
{
	sigset_t set;
	sigemptyset(&set);

	for (size_t i = 0; i < sizeof(stop_signals) / sizeof(stop_signals[0]); ++i)
		sigaddset(&set, stop_signals[i]);

	return signal_set(&set, NULL);
}
