#ifndef __LOG_H__
#define __LOG_H__

#include <netdb.h>
#include <stdio.h>
#include <string.h>

#define print_errno(s)                                                                             \
	{                                                                                          \
		fprintf(stderr, "%s(%d): ", basename(__FILE__), __LINE__);                         \
		perror(s);                                                                         \
	}

#define print_error(...)                                                                           \
	{                                                                                          \
		fprintf(stderr, "%s(%d): ", basename(__FILE__), __LINE__);                         \
		fprintf(stderr, __VA_ARGS__);                                                      \
	}

#endif
