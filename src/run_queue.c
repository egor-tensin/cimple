/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "run_queue.h"
#include "log.h"
#include "msg.h"

#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

struct run {
	int id;
	char *url;
	char *rev;
	SIMPLEQ_ENTRY(run) entries;
};

int run_create(struct run **_entry, int id, const char *_url, const char *_rev)
{
	struct run *entry = malloc(sizeof(struct run));
	if (!entry) {
		log_errno("malloc");
		goto fail;
	}

	char *url = strdup(_url);
	if (!url) {
		log_errno("strdup");
		goto free_entry;
	}

	char *rev = strdup(_rev);
	if (!rev) {
		log_errno("strdup");
		goto free_url;
	}

	entry->id = id;
	entry->url = url;
	entry->rev = rev;

	*_entry = entry;
	return 0;

free_url:
	free(url);

free_entry:
	free(entry);

fail:
	return -1;
}

int run_from_msg(struct run **run, const struct msg *msg)
{
	size_t msg_len = msg_get_length(msg);

	if (msg_len != 3) {
		log_err("Invalid number of arguments for a message: %zu\n", msg_len);
		msg_dump(msg);
		return -1;
	}

	const char **argv = msg_get_strings(msg);
	/* We don't know the ID yet. */
	return run_create(run, 0, argv[1], argv[2]);
}

void run_destroy(struct run *entry)
{
	free(entry->rev);
	free(entry->url);
	free(entry);
}

int run_get_id(const struct run *entry)
{
	return entry->id;
}

const char *run_get_url(const struct run *entry)
{
	return entry->url;
}

const char *run_get_rev(const struct run *entry)
{
	return entry->rev;
}

void run_set_id(struct run *entry, int id)
{
	entry->id = id;
}

void run_queue_create(struct run_queue *queue)
{
	SIMPLEQ_INIT(queue);
}

void run_queue_destroy(struct run_queue *queue)
{
	struct run *entry1 = SIMPLEQ_FIRST(queue);
	while (entry1) {
		struct run *entry2 = SIMPLEQ_NEXT(entry1, entries);
		run_destroy(entry1);
		entry1 = entry2;
	}
	SIMPLEQ_INIT(queue);
}

int run_queue_is_empty(const struct run_queue *queue)
{
	return SIMPLEQ_EMPTY(queue);
}

void run_queue_add_first(struct run_queue *queue, struct run *entry)
{
	SIMPLEQ_INSERT_HEAD(queue, entry, entries);
}

void run_queue_add_last(struct run_queue *queue, struct run *entry)
{
	SIMPLEQ_INSERT_TAIL(queue, entry, entries);
}

struct run *run_queue_remove_first(struct run_queue *queue)
{
	struct run *entry = SIMPLEQ_FIRST(queue);
	SIMPLEQ_REMOVE_HEAD(queue, entries);
	return entry;
}
