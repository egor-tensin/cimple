/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "ci_queue.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

struct ci_queue_entry {
	char *url;
	char *rev;
	STAILQ_ENTRY(ci_queue_entry) entries;
};

int ci_queue_entry_create(struct ci_queue_entry **_entry, const char *_url, const char *_rev)
{
	struct ci_queue_entry *entry;
	char *url, *rev;

	*_entry = malloc(sizeof(struct ci_queue_entry));
	if (!*_entry) {
		log_errno("malloc");
		goto fail;
	}
	entry = *_entry;

	url = strdup(_url);
	if (!url) {
		log_errno("strdup");
		goto free_entry;
	}

	rev = strdup(_rev);
	if (!rev) {
		log_errno("strdup");
		goto free_url;
	}

	entry->url = url;
	entry->rev = rev;

	return 0;

free_url:
	free(url);

free_entry:
	free(entry);

fail:
	return -1;
}

void ci_queue_entry_destroy(struct ci_queue_entry *entry)
{
	free(entry->rev);
	free(entry->url);
	free(entry);
}

const char *ci_queue_entry_get_url(const struct ci_queue_entry *entry)
{
	return entry->url;
}

const char *ci_queue_entry_get_rev(const struct ci_queue_entry *entry)
{
	return entry->rev;
}

void ci_queue_create(struct ci_queue *queue)
{
	STAILQ_INIT(queue);
}

void ci_queue_destroy(struct ci_queue *queue)
{
	struct ci_queue_entry *entry1, *entry2;

	entry1 = STAILQ_FIRST(queue);
	while (entry1) {
		entry2 = STAILQ_NEXT(entry1, entries);
		ci_queue_entry_destroy(entry1);
		entry1 = entry2;
	}
	STAILQ_INIT(queue);
}

int ci_queue_is_empty(const struct ci_queue *queue)
{
	return STAILQ_EMPTY(queue);
}

void ci_queue_add_last(struct ci_queue *queue, struct ci_queue_entry *entry)
{
	STAILQ_INSERT_TAIL(queue, entry, entries);
}

void ci_queue_add_first(struct ci_queue *queue, struct ci_queue_entry *entry)
{
	STAILQ_INSERT_HEAD(queue, entry, entries);
}

struct ci_queue_entry *ci_queue_remove_first(struct ci_queue *queue)
{
	struct ci_queue_entry *entry;

	entry = STAILQ_FIRST(queue);
	STAILQ_REMOVE_HEAD(queue, entries);

	return entry;
}
