#ifndef __CI_QUEUE_H__
#define __CI_QUEUE_H__

#include <sys/queue.h>

struct ci_queue_entry {
	char *url;
	char *rev;
	STAILQ_ENTRY(ci_queue_entry) entries;
};

int ci_queue_entry_create(struct ci_queue_entry **, const char *url, const char *rev);
void ci_queue_entry_destroy(struct ci_queue_entry *);

STAILQ_HEAD(ci_queue, ci_queue_entry);

void ci_queue_create(struct ci_queue *);
void ci_queue_destroy(struct ci_queue *);

int ci_queue_is_empty(const struct ci_queue *);

void ci_queue_push(struct ci_queue *, struct ci_queue_entry *);
void ci_queue_push_head(struct ci_queue *, struct ci_queue_entry *);

struct ci_queue_entry *ci_queue_pop(struct ci_queue *);

#endif
