#!/usr/bin/env bash

# Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
# This file is part of the "cimple" project.
# For details, see https://github.com/egor-tensin/cimple.
# Distributed under the MIT License.

# This script attaches to a set of processes and makes a flame graph using
# flamegraph.pl.

set -o errexit -o nounset -o pipefail
shopt -s inherit_errexit lastpipe

script_name="$( basename -- "${BASH_SOURCE[0]}" )"
readonly script_name

script_usage() {
	local msg
	for msg; do
		echo "$script_name: $msg"
	done

	echo "usage: $script_name OUTPUT_PATH PID [PID...]"
}

join_pids() {
	local result=''
	while [ "$#" -gt 0 ]; do
		if [ -n "$result" ]; then
			result="$result,"
		fi
		result="$result$1"
		shift
	done
	echo "$result"
}

output_dir=''

cleanup() {
	if [ -n "$output_dir" ]; then
		echo "Removing temporary directory: $output_dir"
		rm -rf -- "$output_dir"
	fi
}

make_graph() {
	wait "$record_pid" || true
	perf script -i "$output_dir/perf.data" > "$output_dir/perf.out"
	stackcollapse-perf.pl "$output_dir/perf.out" > "$output_dir/perf.folded"
	flamegraph.pl --width 1400 --color mem "$output_dir/perf.folded" > "$output_path"
}

record_pid=''

stop_record() {
	echo "Stopping 'perf record' process $record_pid"
	kill -SIGINT "$record_pid"
	make_graph
}

check_tools() {
	local tool
	for tool in perf stackcollapse-perf.pl flamegraph.pl; do
		if ! command -v "$tool" &> /dev/null; then
			echo "$script_name: $tool is missing" >&2
			exit 1
		fi
	done
}

main() {
	trap cleanup EXIT
	check_tools

	if [ "$#" -lt 1 ]; then
		script_usage "output path is required" >&2
		exit 1
	fi
	local output_path="$1"
	output_path="$( realpath -- "$output_path" )"
	shift

	if [ "$#" -lt 1 ]; then
		script_usage "at least one process ID is required" >&2
		exit 1
	fi
	local pids
	pids="$( join_pids "$@" )"
	shift

	echo "Output path: $output_path"
	echo "PIDs: $pids"

	output_dir="$( dirname -- "$output_path" )"
	output_dir="$( mktemp -d --tmpdir="$output_dir" )"
	readonly output_dir

	perf record \
		-o "$output_dir/perf.data" \
		--freq=99 \
		--call-graph dwarf,65528 \
		--pid="$pids" \
		--no-inherit &

	record_pid="$!"
	echo "Started 'perf record' process $record_pid"
	trap stop_record SIGINT SIGTERM

	make_graph
}

main "$@"
