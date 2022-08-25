#include "process.h"
#include "log.h"

#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static int wait_for_child(pid_t pid, int *ec)
{
	int status;

	pid_t ret = waitpid(pid, &status, 0);
	if (ret < 0) {
		print_errno("waitpid");
		return ret;
	}

	if (WIFEXITED(status))
		*ec = WEXITSTATUS(status);
	else
		*ec = -1;

	return 0;
}

int proc_spawn(const char *args[], int *ec)
{
	int ret = 0;

	pid_t child_pid = fork();
	if (child_pid < 0) {
		print_errno("fork");
		return child_pid;
	}

	if (child_pid)
		return wait_for_child(child_pid, ec);

	ret = execv(args[0], (char *const *)args);
	if (ret < 0) {
		print_errno("execv");
		exit(ret);
	}

	return 0;
}
