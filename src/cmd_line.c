/*
 * Copyright (c) 2023 Egor Tensin <egor@tensin.name>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "cmd_line.h"
#include "const.h"
#include "file.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *get_current_binary_path(void)
{
	return readlink_wrapper("/proc/self/exe");
}

static char *get_current_binary_name(void)
{
	char *path = get_current_binary_path();
	if (!path)
		return NULL;

	char *name = basename(path);

	char *result = strdup(name);
	if (!result) {
		log_errno("strdup");
		goto free_path;
	}

free_path:
	free(path);

	return result;
}

void exit_with_usage(int ec)
{
	FILE *dest = stdout;
	if (ec)
		dest = stderr;

	char *binary = get_current_binary_name();

	fprintf(dest, "usage: %s %s\n", binary ? binary : "prog", get_usage_string());
	free(binary);
	exit(ec);
}

void exit_with_usage_err(const char *msg)
{
	if (msg)
		fprintf(stderr, "usage error: %s\n", msg);
	exit_with_usage(1);
}

void exit_with_version(void)
{
	char *binary = get_current_binary_name();

	printf("%s v%s (%s)\n", binary ? binary : "prog", project_version, project_rev);
	free(binary);
	exit(0);
}
