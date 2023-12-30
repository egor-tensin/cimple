/*
 * Copyright (c) 2023 Egor Tensin <egor@tensin.name>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __STRING_H__
#define __STRING_H__

/*
 * This is an implementation for stpecpy.
 * For details, see string_copying(7).
 *
 * You can safely do something like this:
 *
 * char buf[128];
 * char *ptr = buf;
 *
 * ptr = string_append(ptr, buf + 128, "random");
 * ptr = string_append(ptr, buf + 128, "strings");
 * ptr = string_append(ptr, buf + 128, "can be appended safely");
 */
char *string_append(char *dst, char *end, const char *src);

int string_to_int(const char *src, int *result);

#endif
