#!/usr/bin/env bash

set -o errexit -o nounset -o pipefail
shopt -s inherit_errexit lastpipe

script_dir="$( dirname -- "${BASH_SOURCE[0]}" )"
script_dir="$( cd -- "$script_dir" && pwd )"

cd -- "$script_dir"

gitdir="$( git rev-parse --git-dir )"
hooks_dir="$gitdir/hooks"

symlink_dest="$( realpath "--relative-to=$hooks_dir" -- "$script_dir/pre-commit.sh" )"

ln -fs -- "$symlink_dest" "$hooks_dir/pre-commit"
