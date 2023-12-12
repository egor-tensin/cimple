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

int my_chdir(const char *dir, char **old);
char *my_readlink(const char *path);

int file_dup(int fd);
void file_close(int fd);

int file_exists(const char *path);
int file_read(int fd, unsigned char **output, size_t *size);

#endif
