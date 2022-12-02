/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
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
		log_errno("execv");
		return ret;
	}

	return ret;
}

static int wait_for_child(pid_t pid, int *ec)
{
	int status;

	pid_t ret = waitpid(pid, &status, 0);
	if (ret < 0) {
		log_errno("waitpid");
		return ret;
	}

	if (WIFEXITED(status))
		*ec = WEXITSTATUS(status);
	else
		*ec = -1;

	return 0;
}

int proc_spawn(const char *args[], const char *envp[], int *ec)
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

	log_errno_if(close(pipe_fds[0]), "close");

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

int proc_capture(const char *args[], const char *envp[], struct proc_output *result)
{
	int pipe_fds[2];
	int ret = 0;

	ret = pipe2(pipe_fds, O_CLOEXEC);
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

	log_errno_if(close(pipe_fds[1]), "close");

	ret = file_read(pipe_fds[0], &result->output, &result->output_len);
	if (ret < 0)
		goto close_pipe;

	ret = wait_for_child(child_pid, &result->ec);
	if (ret < 0)
		goto free_output;

	goto close_pipe;

free_output:
	free(result->output);

close_pipe:
	log_errno_if(close(pipe_fds[0]), "close");
	/* No errno checking here, we might've already closed the write end. */
	close(pipe_fds[1]);

	return ret;
}

void proc_output_init(struct proc_output *output)
{
	output->ec = 0;
	output->output = NULL;
	output->output_len = 0;
}

void proc_output_free(const struct proc_output *output)
{
	free(output->output);
}
