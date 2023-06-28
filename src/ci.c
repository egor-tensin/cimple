/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

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

static const char *ci_env[] = {
    "CI=y",
    "CIMPLE=y",
    NULL,
};
/* clang-format on */

static int ci_run_script(const char *script, struct proc_output *result)
{
	const char *args[] = {script, NULL};
	return proc_capture(args, ci_env, result);
}

int ci_run(struct proc_output *result)
{
	for (const char **script = ci_scripts; *script; ++script) {
		if (!file_exists(*script))
			continue;
		log("Going to run: %s\n", *script);
		return ci_run_script(*script, result);
	}

	log("Couldn't find any CI scripts to run\n");
	return -1;
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
		goto cleanup_repo;

	return ret;

cleanup_repo:
	ci_cleanup_git_repo(*repo);

	return ret;
}

int ci_run_git_repo(const char *url, const char *rev, struct proc_output *output)
{
	char *oldpwd = NULL;
	git_repository *repo = NULL;
	int ret = 0;

	ret = ci_prepare_git_repo(&repo, url, rev);
	if (ret < 0)
		goto exit;

	ret = my_chdir(git_repository_workdir(repo), &oldpwd);
	if (ret < 0)
		goto free_repo;

	ret = ci_run(output);
	if (ret < 0)
		goto oldpwd;

oldpwd:
	my_chdir(oldpwd, NULL);
	free(oldpwd);

free_repo:
	ci_cleanup_git_repo(repo);

exit:
	return ret;
}
