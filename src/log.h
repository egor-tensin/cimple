/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __LOG_H__
#define __LOG_H__

#include "compiler.h"

#include <errno.h>
#include <libgen.h>
#include <stdio.h>

#define LOG_LVL_DEBUG 0
#define LOG_LVL_INFO  1
#define LOG_LVL_ERR   2

extern int g_log_lvl;

int log_entry_start(int lvl, FILE *dest);
void log_entry_end(FILE *dest);

#define log_debug(...)                                                                             \
	do {                                                                                       \
		if (!log_entry_start(LOG_LVL_DEBUG, stdout))                                       \
			break;                                                                     \
		fprintf(stdout, __VA_ARGS__);                                                      \
		log_entry_end(stdout);                                                             \
	} while (0)

#define log(...)                                                                                   \
	do {                                                                                       \
		if (!log_entry_start(LOG_LVL_INFO, stdout))                                        \
			break;                                                                     \
		fprintf(stdout, __VA_ARGS__);                                                      \
		log_entry_end(stdout);                                                             \
	} while (0)

#define _log_err_file_loc()                                                                        \
	do {                                                                                       \
		fprintf(stderr, "%s(%d): ", basename(__FILE__), __LINE__);                         \
	} while (0)

#define log_err(...)                                                                               \
	do {                                                                                       \
		if (!log_entry_start(LOG_LVL_ERR, stderr))                                         \
			break;                                                                     \
		_log_err_file_loc();                                                               \
		fprintf(stderr, __VA_ARGS__);                                                      \
		log_entry_end(stderr);                                                             \
	} while (0)

#define log_errno(fn)                                                                              \
	do {                                                                                       \
		if (!log_entry_start(LOG_LVL_ERR, stderr))                                         \
			break;                                                                     \
		_log_err_file_loc();                                                               \
		perror(fn);                                                                        \
		log_entry_end(stderr);                                                             \
	} while (0)

#define log_errno_if(expr, fn)                                                                     \
	do {                                                                                       \
		int CONCAT(ret, __LINE__) = expr;                                                  \
		if (CONCAT(ret, __LINE__) < 0)                                                     \
			log_errno(fn);                                                             \
	} while (0)

#define pthread_errno(var, fn)                                                                     \
	do {                                                                                       \
		errno = var;                                                                       \
		log_errno(fn);                                                                     \
		var = -var;                                                                        \
	} while (0)

#define pthread_errno_if(expr, fn)                                                                 \
	do {                                                                                       \
		int CONCAT(ret, __LINE__) = expr;                                                  \
		if (CONCAT(ret, __LINE__))                                                         \
			pthread_errno(CONCAT(ret, __LINE__), fn);                                  \
	} while (0)

#endif
