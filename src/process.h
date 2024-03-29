/*
 * Copyright (c) 2022 Egor Tensin <egor@tensin.name>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <stddef.h>

struct proc_output {
	int ec;
	unsigned char *data;
	size_t data_size;
};

/* The exit code is only valid if the functions returns a non-negative number. */
int proc_spawn(const char *args[], const char *envp[], int *ec);

/* Similarly, the contents of the proc_output structure is only valid if the function returns a
 * non-negative number.
 *
 * In that case, you'll need to free the output. */
int proc_capture(const char *args[], const char *envp[], struct proc_output *result);

int proc_output_create(struct proc_output **);
void proc_output_destroy(struct proc_output *);

void proc_output_dump(const struct proc_output *);

#endif
