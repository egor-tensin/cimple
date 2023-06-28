#!/usr/bin/env bash

set -o errexit -o nounset -o pipefail
shopt -s inherit_errexit lastpipe

script_dir="$( dirname -- "${BASH_SOURCE[0]}" )"
script_dir="$( cd -- "$script_dir" && pwd )"
readonly script_dir

ci_output_dir="$( git remote get-url origin )"
ci_output_dir="$ci_output_dir/output"
readonly ci_output_dir

mkdir -p -- "$ci_output_dir"

readonly ci_output_template=ci_XXXXXX

ci_output_path="$( mktemp --tmpdir="$ci_output_dir" "$ci_output_template" )"
readonly ci_output_path

timestamp="$( date --iso-8601=ns )"
readonly timestamp

echo "A CI run happened at $timestamp" | tee -- "$ci_output_path"
