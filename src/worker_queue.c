/*
 * Copyright (c) 2023 Egor Tensin <egor@tensin.name>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "worker_queue.h"
#include "log.h"
#include "net.h"

#include <stdlib.h>
#include <sys/queue.h>

struct worker {
	int fd;
	SIMPLEQ_ENTRY(worker) entries;
};

int worker_create(struct worker **_entry, int fd)
{
	struct worker *entry = malloc(sizeof(struct worker));
	if (!entry) {
		log_errno("malloc");
		return -1;
	}

	entry->fd = fd;

	*_entry = entry;
	return 0;
}

void worker_destroy(struct worker *entry)
{
	net_close(entry->fd);
	free(entry);
}

int worker_get_fd(const struct worker *entry)
{
	return entry->fd;
}

void worker_queue_create(struct worker_queue *queue)
{
	SIMPLEQ_INIT(queue);
}

void worker_queue_destroy(struct worker_queue *queue)
{
	struct worker *entry1 = SIMPLEQ_FIRST(queue);
	while (entry1) {
		struct worker *entry2 = SIMPLEQ_NEXT(entry1, entries);
		worker_destroy(entry1);
		entry1 = entry2;
	}
	SIMPLEQ_INIT(queue);
}

int worker_queue_is_empty(const struct worker_queue *queue)
{
	return SIMPLEQ_EMPTY(queue);
}

void worker_queue_add_first(struct worker_queue *queue, struct worker *entry)
{
	SIMPLEQ_INSERT_HEAD(queue, entry, entries);
}

void worker_queue_add_last(struct worker_queue *queue, struct worker *entry)
{
	SIMPLEQ_INSERT_TAIL(queue, entry, entries);
}

struct worker *worker_queue_remove_first(struct worker_queue *queue)
{
	struct worker *entry = SIMPLEQ_FIRST(queue);
	SIMPLEQ_REMOVE_HEAD(queue, entries);
	return entry;
}
