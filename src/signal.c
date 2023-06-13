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

static int sigterm_signals[] = {SIGINT, SIGTERM, SIGQUIT};

static void sigterms_mask(sigset_t *set)
{
	sigemptyset(set);
	for (size_t i = 0; i < sizeof(sigterm_signals) / sizeof(sigterm_signals[0]); ++i)
		sigaddset(set, sigterm_signals[i]);
}

static int signal_set_mask_internal(const sigset_t *new, sigset_t *old)
{
	int ret = 0;

	ret = pthread_sigmask(SIG_SETMASK, new, old);
	if (ret) {
		pthread_errno(ret, "pthread_sigmask");
		return ret;
	}

	return ret;
}

int signal_set_mask(const sigset_t *new)
{
	return signal_set_mask_internal(new, NULL);
}

int signal_block_all(sigset_t *old)
{
	sigset_t new;
	sigfillset(&new);
	return signal_set_mask_internal(&new, old);
}

int signal_block_sigterms(void)
{
	sigset_t set;
	sigterms_mask(&set);
	return signal_set_mask_internal(&set, NULL);
}

int signalfd_create(const sigset_t *set)
{
	sigset_t old;
	int ret = 0;

	ret = signal_set_mask_internal(set, &old);
	if (ret < 0)
		return ret;

	ret = signalfd(-1, set, SFD_CLOEXEC);
	if (ret < 0)
		goto restore;

	return ret;

restore:
	signal_set_mask_internal(&old, NULL);

	return ret;
}

int signalfd_create_sigterms(void)
{
	sigset_t set;
	sigterms_mask(&set);
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
