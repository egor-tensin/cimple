/*
 * Copyright (c) 2022 Egor Tensin <egor@tensin.name>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <stddef.h>

struct process_output {
	int ec;
	unsigned char *data;
	size_t data_size;
};

/* The exit code is only valid if the functions returns a non-negative number. */
int process_execute(const char *args[], const char *envp[], int *ec);

/* Similarly, the contents of the process_output structure is only valid if the function returns a
 * non-negative number.
 *
 * In that case, you'll need to free the output. */
int process_execute_and_capture(const char *args[], const char *envp[],
                                struct process_output *result);

int process_output_create(struct process_output **);
void process_output_destroy(struct process_output *);

void process_output_dump(const struct process_output *);

#endif
