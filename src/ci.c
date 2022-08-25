#include "ci.h"
#include "file.h"
#include "git.h"
#include "log.h"
#include "process.h"

#include <git2.h>
#include <stddef.h>
#include <stdlib.h>

/* clang-format off */
static const char *ci_scripts[] = {
    "./.ci.sh",
    "./.ci",
    "./ci.sh",
    "./ci",
    NULL,
};
/* clang-format on */

static run_result ci_run_script(const char *script, int *ec)
{
	const char *args[] = {script, NULL};
	int ret = 0;

	ret = proc_spawn(args, ec);
	if (ret < 0)
		return RUN_ERROR;
	if (*ec)
		return RUN_FAILURE;
	return RUN_SUCCESS;
}

run_result ci_run(int *ec)
{
	for (const char **script = ci_scripts; *script; ++script) {
		if (!file_exists(*script))
			continue;
		print_log("Going to run: %s\n", *script);
		return ci_run_script(*script, ec);
	}

	print_log("Couldn't find any CI scripts to run\n");
	return RUN_NO;
}

static void ci_cleanup_git_repo(git_repository *repo)
{
	rm_rf(git_repository_workdir(repo));
	libgit_repository_free(repo);
}

static int ci_prepare_git_repo(git_repository **repo, const char *url, const char *rev)
{
	int ret = 0;

	ret = libgit_clone_to_tmp(repo, url);
	if (ret < 0)
		return ret;

	ret = libgit_checkout(*repo, rev);
	if (ret < 0)
		goto free_repo;

	return ret;

free_repo:
	ci_cleanup_git_repo(*repo);

	return ret;
}

run_result ci_run_git_repo(const char *url, const char *rev, int *ec)
{
	char *oldpwd;
	git_repository *repo;
	run_result result = RUN_ERROR;
	int ret = 0;

	ret = ci_prepare_git_repo(&repo, url, rev);
	if (ret < 0)
		goto exit;

	ret = my_chdir(git_repository_workdir(repo), &oldpwd);
	if (ret < 0)
		goto free_repo;

	result = ci_run(ec);
	if (result < 0)
		goto oldpwd;

oldpwd:
	my_chdir(oldpwd, NULL);
	free(oldpwd);

free_repo:
	ci_cleanup_git_repo(repo);

exit:
	return result;
}
