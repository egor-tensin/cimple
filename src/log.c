/*
 * Copyright (c) 2023 Egor Tensin <egor@tensin.name>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "log.h"

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

int g_log_lvl = LOG_LVL_INFO;

static inline void log_prefix_timestamp(FILE *dest)
{
	struct timeval tv;
	struct tm tm;
	char buf[64];
	size_t used = 0;

	if (gettimeofday(&tv, NULL) < 0)
		return;
	if (!gmtime_r(&tv.tv_sec, &tm))
		return;

	buf[0] = '\0';
	used += strftime(buf + used, sizeof(buf) - used, "%F %T", &tm);
	long long msec = (long long)tv.tv_usec / 1000;
	used += snprintf(buf + used, sizeof(buf) - used, ".%03lld | ", msec);
	fprintf(dest, "%s", buf);
}

static inline void log_prefix_thread_id(FILE *dest)
{
	fprintf(dest, "%d | ", gettid());
}

int log_entry_start(int lvl, FILE *dest)
{
	if (lvl < g_log_lvl)
		return 0;
	flockfile(dest);
	log_prefix_timestamp(dest);
	log_prefix_thread_id(dest);
	return 1;
}

void log_entry_end(FILE *dest)
{
	funlockfile(dest);
}
