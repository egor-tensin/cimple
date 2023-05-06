/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __SIGNAL_H__
#define __SIGNAL_H__

#include <signal.h>

extern volatile sig_atomic_t global_stop_flag;
int signal_install_global_handler();

int signal_set(const sigset_t *new, sigset_t *old);

int signal_block_parent(sigset_t *old);
int signal_block_child();

#endif
