/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __LOG_H__
#define __LOG_H__

#include <errno.h>
#include <libgen.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

static inline void log_print_timestamp(FILE *dest)
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
	used += strftime(buf + used, sizeof(buf) - used, "[%F %T", &tm);
	used += snprintf(buf + used, sizeof(buf) - used, ".%03ld] ", tv.tv_usec / 1000);
	fprintf(dest, "%s", buf);
}

#define CONCAT_INNER(a, b) a##b
#define CONCAT(a, b) CONCAT_INNER(a, b)

static inline void log_prefix(FILE *dest)
{
	log_print_timestamp(dest);
}

#define log_err_prefix()                                                                           \
	do {                                                                                       \
		log_prefix(stderr);                                                                \
		fprintf(stderr, "%s(%d): ", basename(__FILE__), __LINE__);                         \
	} while (0)

#define log(...)                                                                                   \
	do {                                                                                       \
		log_prefix(stdout);                                                                \
		printf(__VA_ARGS__);                                                               \
	} while (0)

#define log_err(...)                                                                               \
	do {                                                                                       \
		log_err_prefix();                                                                  \
		fprintf(stderr, __VA_ARGS__);                                                      \
	} while (0)

#define log_errno(s)                                                                               \
	do {                                                                                       \
		log_err_prefix();                                                                  \
		perror(s);                                                                         \
	} while (0)

#define log_errno_if(expr, s)                                                                      \
	do {                                                                                       \
		int CONCAT(ret, __LINE__) = expr;                                                  \
		if (CONCAT(ret, __LINE__) < 0)                                                     \
			log_errno(s);                                                              \
	} while (0)

#define pthread_errno(var, s)                                                                      \
	do {                                                                                       \
		errno = var;                                                                       \
		log_errno(s);                                                                      \
		var = -var;                                                                        \
	} while (0)

#define pthread_errno_if(expr, s)                                                                  \
	do {                                                                                       \
		int CONCAT(ret, __LINE__) = expr;                                                  \
		if (CONCAT(ret, __LINE__))                                                         \
			pthread_errno(CONCAT(ret, __LINE__), s);                                   \
	} while (0)

#endif
