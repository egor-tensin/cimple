/*
 * Copyright (c) 2023 Egor Tensin <egor@tensin.name>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "string.h"
#include "log.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* glibc calls this stpecpy; it's not provided by glibc; however, it does
 * provide a possible implementation in string_copying(7), which I copied from. */
char *string_append(char *dst, char *end, const char *src)
{
	if (!dst)
		return NULL;
	if (dst == end)
		return end;

	char *p = memccpy(dst, src, '\0', end - dst);
	if (p)
		return p - 1;

	end[-1] = '\0';
	return end;
}

int string_to_int(const char *src, int *result)
{
	char *endptr = NULL;

	errno = 0;
	long ret = strtol(src, &endptr, 10);

	if (errno) {
		log_errno("strtol");
		return -1;
	}

	if (endptr == src || *endptr != '\0') {
		log_err("Invalid number: %s\n", src);
		return -1;
	}

	*result = (int)ret;
	return 0;
}
