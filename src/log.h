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

#define log_error_prefix()                                                                         \
	do {                                                                                       \
		log_prefix(stderr);                                                                \
		fprintf(stderr, "%s(%d): ", basename(__FILE__), __LINE__);                         \
	} while (0)

#define print_errno(s)                                                                             \
	do {                                                                                       \
		log_error_prefix();                                                                \
		perror(s);                                                                         \
	} while (0)

#define check_errno(expr, s)                                                                       \
	do {                                                                                       \
		int CONCAT(ret, __LINE__) = expr;                                                  \
		if (CONCAT(ret, __LINE__) < 0)                                                     \
			print_errno(s);                                                            \
	} while (0)

#define pthread_print_errno(var, s)                                                                \
	do {                                                                                       \
		errno = var;                                                                       \
		print_errno(s);                                                                    \
		var = -var;                                                                        \
	} while (0)

#define pthread_check(expr, s)                                                                     \
	do {                                                                                       \
		int CONCAT(ret, __LINE__) = expr;                                                  \
		if (CONCAT(ret, __LINE__))                                                         \
			pthread_print_errno(CONCAT(ret, __LINE__), s);                             \
	} while (0)

#define print_error(...)                                                                           \
	do {                                                                                       \
		log_error_prefix();                                                                \
		fprintf(stderr, __VA_ARGS__);                                                      \
	} while (0)

#define print_log(...)                                                                             \
	do {                                                                                       \
		log_prefix(stdout);                                                                \
		printf(__VA_ARGS__);                                                               \
	} while (0)

#endif
