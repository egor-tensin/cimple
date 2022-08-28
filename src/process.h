#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <stddef.h>

struct proc_output {
	int ec;
	char *output;
	size_t output_len;
};

/* The exit code is only valid if the functions returns a non-negative number. */
int proc_spawn(const char *args[], int *ec);

/* Similarly, the contents of the proc_output structure is only valid if the function returns a
 * non-negative number.
 *
 * In that case, you'll need to free the output. */
int proc_capture(const char *args[], struct proc_output *result);

void proc_output_init(struct proc_output *);
void proc_output_free(const struct proc_output *);

#endif
