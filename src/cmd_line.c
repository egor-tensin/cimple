/*
 * Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "cmd_line.h"

#include <stdio.h>
#include <stdlib.h>

void exit_with_usage(int ec, const char *argv0)
{
	FILE *dest = stdout;
	if (ec)
		dest = stderr;

	fprintf(dest, "usage: %s %s\n", argv0, get_usage_string());
	exit(ec);
}

void exit_with_usage_err(const char *argv0, const char *msg)
{
	if (msg)
		fprintf(stderr, "usage error: %s\n", msg);
	exit_with_usage(1, argv0);
}

void exit_with_version()
{
	printf("%s\n", VERSION);
	exit(0);
}
