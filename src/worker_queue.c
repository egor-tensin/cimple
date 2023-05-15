/*
 * Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "worker_queue.h"
#include "log.h"

#include <pthread.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <unistd.h>

struct worker {
	pthread_t thread;
	int fd;
	STAILQ_ENTRY(worker) entries;
};

int worker_create(struct worker **_entry, int fd)
{
	struct worker *entry = malloc(sizeof(struct worker));
	if (!entry) {
		log_errno("malloc");
		return -1;
	}

	entry->thread = pthread_self();
	entry->fd = fd;

	*_entry = entry;
	return 0;
}

void worker_destroy(struct worker *entry)
{
	log("Waiting for worker %d thread to exit\n", entry->fd);
	pthread_errno_if(pthread_join(entry->thread, NULL), "pthread_join");
	log_errno_if(close(entry->fd), "close");
	free(entry);
}

int worker_get_fd(const struct worker *entry)
{
	return entry->fd;
}

void worker_queue_create(struct worker_queue *queue)
{
	STAILQ_INIT(queue);
}

void worker_queue_destroy(struct worker_queue *queue)
{
	struct worker *entry1 = STAILQ_FIRST(queue);
	while (entry1) {
		struct worker *entry2 = STAILQ_NEXT(entry1, entries);
		worker_destroy(entry1);
		entry1 = entry2;
	}
	STAILQ_INIT(queue);
}

int worker_queue_is_empty(const struct worker_queue *queue)
{
	return STAILQ_EMPTY(queue);
}

void worker_queue_add_first(struct worker_queue *queue, struct worker *entry)
{
	STAILQ_INSERT_HEAD(queue, entry, entries);
}

void worker_queue_add_last(struct worker_queue *queue, struct worker *entry)
{
	STAILQ_INSERT_HEAD(queue, entry, entries);
}

struct worker *worker_queue_remove_first(struct worker_queue *queue)
{
	struct worker *entry = STAILQ_FIRST(queue);
	STAILQ_REMOVE_HEAD(queue, entries);
	return entry;
}