/*
 * Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "protocol.h"
#include "const.h"
#include "log.h"
#include "msg.h"
#include "process.h"
#include "run_queue.h"
#include "string.h"

#include <stddef.h>
#include <stdio.h>

static int check_msg_length(const struct msg *msg, size_t expected)
{
	size_t actual = msg_get_length(msg);

	if (actual != expected) {
		log_err("Invalid number of arguments for a message: %zu\n", actual);
		msg_dump(msg);
		return -1;
	}

	return 0;
}

int msg_run_parse(const struct msg *msg, struct run **run)
{
	int ret = check_msg_length(msg, 3);
	if (ret < 0)
		return ret;

	const char **argv = msg_get_strings(msg);
	/* We don't know the ID yet. */
	return run_create(run, 0, argv[1], argv[2]);
}

int msg_new_worker_create(struct msg **msg)
{
	static const char *argv[] = {CMD_NEW_WORKER, NULL};
	return msg_from_argv(msg, argv);
}

int msg_start_create(struct msg **msg, const struct run *run)
{
	char id[16];
	snprintf(id, sizeof(id), "%d", run_get_id(run));

	const char *argv[] = {CMD_START, id, run_get_url(run), run_get_rev(run), NULL};

	return msg_from_argv(msg, argv);
}

int msg_start_parse(const struct msg *msg, struct run **run)
{
	int ret = 0;

	ret = check_msg_length(msg, 4);
	if (ret < 0)
		return ret;

	const char **argv = msg_get_strings(msg);

	int id = 0;

	ret = string_to_int(argv[1], &id);
	if (ret < 0)
		return ret;

	return run_create(run, id, argv[2], argv[3]);
}

int msg_finished_create(struct msg **msg, int run_id, const struct proc_output *output)
{
	char id[16];
	char ec[16];

	snprintf(id, sizeof(id), "%d", run_id);
	snprintf(ec, sizeof(ec), "%d", output->ec);

	const char *argv[] = {CMD_FINISHED, id, ec, NULL};

	return msg_from_argv(msg, argv);
}

int msg_finished_parse(const struct msg *msg, int *run_id, struct proc_output *output)
{
	int ret = 0;

	ret = check_msg_length(msg, 3);
	if (ret < 0)
		return ret;

	const char **argv = msg_get_strings(msg);

	proc_output_init(output);

	ret = string_to_int(argv[1], run_id);
	if (ret < 0)
		return ret;
	ret = string_to_int(argv[2], &output->ec);
	if (ret < 0)
		return ret;

	return 0;
}
