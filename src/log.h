#ifndef __LOG_H__
#define __LOG_H__

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

#define CONCAT_INNER(a, b) a##b
#define CONCAT(a, b) CONCAT_INNER(a, b)

#define print_errno(s)                                                                             \
	{                                                                                          \
		fprintf(stderr, "%s(%d): ", basename(__FILE__), __LINE__);                         \
		perror(s);                                                                         \
	}

#define pthread_print_errno(var, s)                                                                \
	{                                                                                          \
		errno = var;                                                                       \
		print_errno(s);                                                                    \
		var = -var;                                                                        \
	}

#define pthread_check(expr, s)                                                                     \
	{                                                                                          \
		int CONCAT(ret, __LINE__) = expr;                                                  \
		if (CONCAT(ret, __LINE__))                                                         \
			pthread_print_errno(CONCAT(ret, __LINE__), s);                             \
	}

#define print_error(...)                                                                           \
	{                                                                                          \
		fprintf(stderr, "%s(%d): ", basename(__FILE__), __LINE__);                         \
		fprintf(stderr, __VA_ARGS__);                                                      \
	}

#define print_log(...) printf(__VA_ARGS__)

#endif
