/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __SIGNAL_H__
#define __SIGNAL_H__

#include "compiler.h"

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

int signal_set(const sigset_t *new, sigset_t *old);

int signal_block_parent(sigset_t *old);
int signal_block_child();

#endif
