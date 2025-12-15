#!/usr/bin/env bash

set -o errexit -o nounset -o pipefail
shopt -s inherit_errexit lastpipe

test_build() {
	docker compose --progress plain build --pull "$@"
}

test_build_clang() {
	echo ----------------------------------------------------------------------
	echo Building w/ clang
	echo ----------------------------------------------------------------------
	echo
	test_build --build-arg COMPILER=clang
}

test_build_gcc() {
	echo ----------------------------------------------------------------------
	echo Building w/ gcc
	echo ----------------------------------------------------------------------
	echo
	test_build --build-arg COMPILER=gcc
}

main() {
	test_build_gcc
	test_build_clang
}

main
