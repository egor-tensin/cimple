/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "signal.h"
#include "compiler.h"
#include "event_loop.h"
#include "file.h"
#include "log.h"

#include <poll.h>
#include <signal.h>
#include <stddef.h>
#include <sys/signalfd.h>

static int stop_signals[] = {SIGINT, SIGTERM, SIGQUIT};

static void stops_set(sigset_t *set)
{
	sigemptyset(set);
	for (size_t i = 0; i < sizeof(stop_signals) / sizeof(stop_signals[0]); ++i)
		sigaddset(set, stop_signals[i]);
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

int signal_block_stops(void)
{
	sigset_t set;
	stops_set(&set);
	return signal_set(&set, NULL);
}

int signal_restore(const sigset_t *new)
{
	return signal_set(new, NULL);
}

int signalfd_create(const sigset_t *set)
{
	sigset_t old;
	int ret = 0;

	ret = signal_set(set, &old);
	if (ret < 0)
		return ret;

	ret = signalfd(-1, set, SFD_CLOEXEC);
	if (ret < 0)
		goto restore;

	return ret;

restore:
	signal_set(&old, NULL);

	return ret;
}

int signalfd_listen_for_stops(void)
{
	sigset_t set;
	stops_set(&set);
	return signalfd_create(&set);
}

void signalfd_destroy(int fd)
{
	file_close(fd);
}

int signalfd_add_to_event_loop(int fd, struct event_loop *loop, event_handler handler, void *arg)
{
	return event_loop_add(loop, fd, POLLIN, handler, arg);
}
