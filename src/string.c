/*
 * Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "string.h"

#include <string.h>

char *stpecpy(char *dst, char *end, const char *src)
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
