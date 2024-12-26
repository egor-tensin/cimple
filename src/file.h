/*
 * Copyright (c) 2022 Egor Tensin <egor@tensin.name>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __FILE_H__
#define __FILE_H__

#include <stddef.h>

int rm_rf(const char *dir);

/* This chdir(2) wrapper optionally saves the previous working directory in the
 * `old` pointer, allowing the user to switch back to it if necessary. */
int chdir_wrapper(const char *dir, char **old);

/* This readlink(2) wrapper allocates enough memory dynamically. */
char *readlink_wrapper(const char *path);

int file_dup(int fd);
void file_close(int fd);

int file_exists(const char *path);
int file_read(int fd, unsigned char **output, size_t *size);

#endif
