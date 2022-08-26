#include "worker_queue.h"
#include "log.h"

#include <stdlib.h>
#include <sys/queue.h>
#include <unistd.h>

int worker_queue_entry_create(struct worker_queue_entry **entry, int fd)
{
	int newfd = dup(fd);

	if (newfd < 0) {
		print_errno("malloc");
		return -1;
	}

	*entry = malloc(sizeof(struct worker_queue_entry));
	if (!*entry) {
		print_errno("malloc");
		goto close_newfd;
	}
	(*entry)->fd = newfd;

	return 0;

close_newfd:
	check_errno(close(newfd), "close");

	return -1;
}

void worker_queue_entry_destroy(struct worker_queue_entry *entry)
{
	check_errno(close(entry->fd), "close");
	free(entry);
}

void worker_queue_create(struct worker_queue *queue)
{
	STAILQ_INIT(queue);
}

void worker_queue_destroy(struct worker_queue *queue)
{
	struct worker_queue_entry *entry1, *entry2;

	entry1 = STAILQ_FIRST(queue);
	while (entry1) {
		entry2 = STAILQ_NEXT(entry1, entries);
		worker_queue_entry_destroy(entry1);
		entry1 = entry2;
	}
	STAILQ_INIT(queue);
}

int worker_queue_is_empty(const struct worker_queue *queue)
{
	return STAILQ_EMPTY(queue);
}

void worker_queue_push(struct worker_queue *queue, struct worker_queue_entry *entry)
{
	STAILQ_INSERT_TAIL(queue, entry, entries);
}

void worker_queue_push_head(struct worker_queue *queue, struct worker_queue_entry *entry)
{
	STAILQ_INSERT_HEAD(queue, entry, entries);
}

struct worker_queue_entry *worker_queue_pop(struct worker_queue *queue)
{
	struct worker_queue_entry *entry;

	entry = STAILQ_FIRST(queue);
	STAILQ_REMOVE_HEAD(queue, entries);

	return entry;
}
