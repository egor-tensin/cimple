#ifndef __WORKER_QUEUE_H__
#define __WORKER_QUEUE_H__

#include <sys/queue.h>

struct worker_queue_entry {
	int fd;
	STAILQ_ENTRY(worker_queue_entry) entries;
};

int worker_queue_entry_create(struct worker_queue_entry **, int fd);
void worker_queue_entry_destroy(struct worker_queue_entry *);

STAILQ_HEAD(worker_queue, worker_queue_entry);

void worker_queue_create(struct worker_queue *);
void worker_queue_destroy(struct worker_queue *);

int worker_queue_is_empty(const struct worker_queue *);

void worker_queue_push(struct worker_queue *, struct worker_queue_entry *);
void worker_queue_push_head(struct worker_queue *, struct worker_queue_entry *);

struct worker_queue_entry *worker_queue_pop(struct worker_queue *);

#endif
