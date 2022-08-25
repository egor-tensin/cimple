#include "ci_queue.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

int ci_queue_entry_create(struct ci_queue_entry **entry, const char *_url, const char *_rev)
{
	char *url, *rev;

	url = strdup(_url);
	if (!url) {
		print_errno("strdup");
		goto fail;
	}

	rev = strdup(_rev);
	if (!rev) {
		print_errno("strdup");
		goto free_url;
	}

	*entry = malloc(sizeof(struct ci_queue_entry));
	if (!*entry) {
		print_errno("malloc");
		goto free_rev;
	}
	(*entry)->url = url;
	(*entry)->rev = rev;

	return 0;

free_rev:
	free(rev);

free_url:
	free(url);

fail:
	return -1;
}

void ci_queue_entry_destroy(struct ci_queue_entry *entry)
{
	free(entry->rev);
	free(entry->url);
	free(entry);
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

void ci_queue_push(struct ci_queue *queue, struct ci_queue_entry *entry)
{
	STAILQ_INSERT_TAIL(queue, entry, entries);
}

void ci_queue_push_head(struct ci_queue *queue, struct ci_queue_entry *entry)
{
	STAILQ_INSERT_HEAD(queue, entry, entries);
}

struct ci_queue_entry *ci_queue_pop(struct ci_queue *queue)
{
	struct ci_queue_entry *entry;

	entry = STAILQ_FIRST(queue);
	STAILQ_REMOVE_HEAD(queue, entries);

	return entry;
}
