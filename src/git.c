#include "git.h"
#include "file.h"
#include "log.h"

#include <git2.h>

#define git_print_error(fn)                                                                        \
	do {                                                                                       \
		const git_error *error = git_error_last();                                         \
		const char *msg = error && error->message ? error->message : "???";                \
		print_error("%s: %s\n", fn, msg);                                                  \
	} while (0)

int libgit_init()
{
	int ret = 0;

	ret = git_libgit2_init();
	if (ret < 0) {
		git_print_error("git_libgit2_init");
		return ret;
	}

	return 0;
}

void libgit_shutdown()
{
	git_libgit2_shutdown();
}

int libgit_clone(git_repository **repo, const char *url, const char *dir)
{
	git_clone_options opts;
	int ret = 0;

	print_log("Cloning git repository from %s to %s\n", url, dir);

	ret = git_clone_options_init(&opts, GIT_CLONE_OPTIONS_VERSION);
	if (ret < 0) {
		git_print_error("git_clone_options_init");
		return ret;
	}
	opts.checkout_opts.checkout_strategy = GIT_CHECKOUT_NONE;

	ret = git_clone(repo, url, dir, &opts);
	if (ret < 0) {
		git_print_error("git_clone");
		return ret;
	}

	return 0;
}

int libgit_clone_to_tmp(git_repository **repo, const char *url)
{
	char dir[] = "/tmp/git.XXXXXX";

	if (!mkdtemp(dir)) {
		print_errno("mkdtemp");
		return -1;
	}

	return libgit_clone(repo, url, dir);
}

void libgit_repository_free(git_repository *repo)
{
	git_repository_free(repo);
}

int libgit_checkout(git_repository *repo, const char *rev)
{
	git_checkout_options opts;
	git_object *obj;
	int ret = 0;

	print_log("Checking out revision %s\n", rev);

	ret = git_revparse_single(&obj, repo, rev);
	if (ret < 0) {
		git_print_error("git_revparse_single");
		return ret;
	}

	ret = git_checkout_options_init(&opts, GIT_CHECKOUT_OPTIONS_VERSION);
	if (ret < 0) {
		git_print_error("git_checkout_options_init");
		goto free_obj;
	}
	opts.checkout_strategy = GIT_CHECKOUT_FORCE;

	ret = git_checkout_tree(repo, obj, &opts);
	if (ret < 0) {
		git_print_error("git_checkout_tree");
		goto free_obj;
	}

	ret = git_repository_set_head_detached(repo, git_object_id(obj));
	if (ret < 0) {
		git_print_error("git_repository_set_head_detached");
		goto free_obj;
	}

free_obj:
	git_object_free(obj);

	return ret;
}
