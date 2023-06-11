/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __SIGNAL_H__
#define __SIGNAL_H__

#include "event_loop.h"

#include <signal.h>

extern volatile sig_atomic_t global_stop_flag;
int signal_handle_stops(void);

int signal_block_all(sigset_t *old);
int signal_block_stops(void);

int signal_restore(const sigset_t *new);

int signalfd_create(const sigset_t *);
void signalfd_destroy(int fd);

int signalfd_add_to_event_loop(int fd, struct event_loop *, event_loop_handler handler, void *arg);

int signalfd_listen_for_stops(void);

#endif
