#ifndef __GIT_H__
#define __GIT_H__

#include <git2.h>

int libgit_init();
void libgit_shutdown();

/* These never check out any files (so that we don't have to do 2 checkouts). */
int libgit_clone(git_repository **, const char *url, const char *dir);
int libgit_clone_to_tmp(git_repository **, const char *url);

/* Free a cloned repository. */
void libgit_repository_free(git_repository *);

/* I tried to make this an equivalent of `git checkout`. */
int libgit_checkout(git_repository *, const char *rev);

#endif
