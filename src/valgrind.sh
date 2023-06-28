#!/usr/bin/env bash

set -o errexit -o nounset -o pipefail
shopt -s inherit_errexit lastpipe

if ! command -v valgrind &> /dev/null; then
    echo 'Please make sure valgrind is available.' >&2
    exit 1
fi

exec valgrind -q \
    --error-exitcode=10 \
    --leak-check=full \
    --show-leak-kinds=all \
    --track-origins=yes \
    --track-fds=yes \
    --trace-children=yes \
    -- "$@"
