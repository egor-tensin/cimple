#ifndef __CI_H__
#define __CI_H__

#include "process.h"

int ci_run(struct proc_output *);

/*
 * This is a high-level function. It's basically equivalent to the following
 * sequence in bash:
 *
 *     dir="$( mktemp -d )"
 *     git clone --no-checkout "$url" "$dir"
 *     pushd "$dir"
 *     git checkout "$rev"
 *     ./ci
 *     popd
 *     rm -rf "$dir"
 *
 */
int ci_run_git_repo(const char *url, const char *rev, struct proc_output *);

#endif
