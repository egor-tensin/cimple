/*
 * Copyright (c) 2022 Egor Tensin <egor@tensin.name>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "process.h"
#include "file.h"
#include "log.h"

#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static int exec_child(const char *args[], const char *envp[])
{
	static const char *default_envp[] = {NULL};

	if (!envp)
		envp = default_envp;

	int ret = execvpe(args[0], (char *const *)args, (char *const *)envp);
	if (ret < 0) {
		log_errno("execvpe");
		return ret;
	}

	return ret;
}

static int wait_for_child(pid_t pid, int *ec)
{
	int status;

	pid_t ret = waitpid(pid, &status, __WNOTHREAD);
	if (ret < 0) {
		log_errno("waitpid");
		return ret;
	}

	/* The child process reports the lowest 8 bits of its exit code, which
	 * are treated as an unsigned integer on Linux.
	 *
	 * If it was killed by a signal, indicate that by negating the signal
	 * number. */

	if (WIFEXITED(status)) {
		*ec = WEXITSTATUS(status);
	} else if (WIFSIGNALED(status)) {
		*ec = -WTERMSIG(status);
	} else {
		log_err("This shouldn't happen: %d\n", status);
		*ec = 1;
	}

	return 0;
}

int process_execute(const char *args[], const char *envp[], int *ec)
{
	pid_t child_pid = fork();
	if (child_pid < 0) {
		log_errno("fork");
		return child_pid;
	}

	if (!child_pid)
		exit(exec_child(args, envp));

	return wait_for_child(child_pid, ec);
}

static int redirect_and_exec_child(int pipe_fds[2], const char *args[], const char *envp[])
{
	int ret = 0;

	file_close(pipe_fds[0]);

	ret = dup2(pipe_fds[1], STDOUT_FILENO);
	if (ret < 0) {
		log_errno("dup2");
		return ret;
	}

	ret = dup2(pipe_fds[1], STDERR_FILENO);
	if (ret < 0) {
		log_errno("dup2");
		return ret;
	}

	return exec_child(args, envp);
}

int process_execute_and_capture(const char *args[], const char *envp[],
                                struct process_output *result)
{
	static const int flags = O_CLOEXEC;
	int pipe_fds[2];
	int ret = 0;

	ret = pipe2(pipe_fds, flags);
	if (ret < 0) {
		log_errno("pipe2");
		return -1;
	}

	pid_t child_pid = fork();
	if (child_pid < 0) {
		log_errno("fork");
		goto close_pipe;
	}

	if (!child_pid)
		exit(redirect_and_exec_child(pipe_fds, args, envp));

	file_close(pipe_fds[1]);

	ret = file_read(pipe_fds[0], &result->data, &result->data_size);
	if (ret < 0)
		goto close_pipe;

	ret = wait_for_child(child_pid, &result->ec);
	if (ret < 0)
		goto free_data;

	goto close_pipe;

free_data:
	free(result->data);

close_pipe:
	file_close(pipe_fds[0]);
	/* No errno checking here, we might've already closed the write end. */
	close(pipe_fds[1]);

	return ret;
}

int process_output_create(struct process_output **_output)
{
	struct process_output *output = calloc(1, sizeof(struct process_output));
	if (!output) {
		log_errno("calloc");
		return -1;
	}

	output->ec = 0;
	output->data = NULL;
	output->data_size = 0;

	*_output = output;
	return 0;
}

void process_output_destroy(struct process_output *output)
{
	free(output->data);
	free(output);
}

void process_output_dump(const struct process_output *output)
{
	log("Process exit code: %d\n", output->ec);
	log("Process output: %zu bytes\n", output->data_size);
}
