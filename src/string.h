/*
 * Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __STRING_H__
#define __STRING_H__

/* For details, see string_copying(7). */
char *stpecpy(char *dst, char *end, const char *src);

int string_to_int(const char *src, int *result);

#endif
