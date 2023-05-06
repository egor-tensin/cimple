#!/usr/bin/env bash

set -o errexit -o nounset -o pipefail
shopt -s inherit_errexit lastpipe

if ! command -v valgrind &> /dev/null; then
    echo 'Please make sure valgrind is available.' >&2
    exit 1
fi

valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose "$@"
