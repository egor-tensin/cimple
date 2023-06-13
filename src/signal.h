/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __SIGNAL_H__
#define __SIGNAL_H__

#include <signal.h>

int signal_set_mask(const sigset_t *new);

int signal_block_all(sigset_t *old);
int signal_block_sigterms(void);

int signalfd_create(const sigset_t *);
int signalfd_create_sigterms(void);
void signalfd_destroy(int fd);

#endif
