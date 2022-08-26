#ifndef __LOG_H__
#define __LOG_H__

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

#define CONCAT_INNER(a, b) a##b
#define CONCAT(a, b) CONCAT_INNER(a, b)

#define print_errno(s)                                                                             \
	do {                                                                                       \
		fprintf(stderr, "%s(%d): ", basename(__FILE__), __LINE__);                         \
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
		fprintf(stderr, "%s(%d): ", basename(__FILE__), __LINE__);                         \
		fprintf(stderr, __VA_ARGS__);                                                      \
	} while (0)

#define print_log(...) printf(__VA_ARGS__)

#endif
