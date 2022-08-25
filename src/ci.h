#ifndef __CI_H__
#define __CI_H__

typedef enum {
	RUN_ERROR = -1,
	RUN_SUCCESS,
	RUN_FAILURE,
	RUN_NO,
} run_result;

run_result ci_run(int *ec);

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
run_result ci_run_git_repo(const char *url, const char *rev, int *ec);

#endif
