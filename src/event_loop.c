/*
 * Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "event_loop.h"
#include "log.h"
#include "net.h"
#include "string.h"

#include <poll.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/queue.h>

SIMPLEQ_HEAD(event_fd_queue, event_fd);

struct event_fd {
	int fd;
	short events;
	event_handler handler;
	void *arg;
	int once;

	SIMPLEQ_ENTRY(event_fd) entries;
};

static struct event_fd *event_fd_create(int fd, short events, event_handler handler, void *arg)
{
	struct event_fd *res = calloc(1, sizeof(struct event_fd));
	if (!res) {
		log_errno("malloc");
		return NULL;
	}

	res->fd = fd;
	res->events = events;
	res->handler = handler;
	res->arg = arg;
	res->once = 0;

	return res;
}

static void event_fd_destroy(struct event_fd *entry)
{
	free(entry);
}

static void event_fd_queue_create(struct event_fd_queue *queue)
{
	SIMPLEQ_INIT(queue);
}

static void event_fd_queue_destroy(struct event_fd_queue *queue)
{
	struct event_fd *entry1 = SIMPLEQ_FIRST(queue);
	while (entry1) {
		struct event_fd *entry2 = SIMPLEQ_NEXT(entry1, entries);
		event_fd_destroy(entry1);
		entry1 = entry2;
	}
	SIMPLEQ_INIT(queue);
}

struct event_loop {
	nfds_t nfds;
	struct event_fd_queue entries;
};

int event_loop_create(struct event_loop **_loop)
{
	int ret = 0;

	struct event_loop *loop = calloc(1, sizeof(struct event_loop));
	if (!loop) {
		log_errno("calloc");
		return -1;
	}
	*_loop = loop;

	event_fd_queue_create(&loop->entries);

	return ret;
}

void event_loop_destroy(struct event_loop *loop)
{
	event_fd_queue_destroy(&loop->entries);
	free(loop);
}

static void event_loop_add_internal(struct event_loop *loop, struct event_fd *entry)
{
	log("Adding descriptor %d to event loop\n", entry->fd);

	nfds_t nfds = loop->nfds + 1;
	SIMPLEQ_INSERT_TAIL(&loop->entries, entry, entries);
	loop->nfds = nfds;
}

int event_loop_add(struct event_loop *loop, int fd, short events, event_handler handler, void *arg)
{
	struct event_fd *entry = event_fd_create(fd, events, handler, arg);
	if (!entry)
		return -1;
	event_loop_add_internal(loop, entry);
	return 0;
}

int event_loop_add_once(struct event_loop *loop, int fd, short events, event_handler handler,
                        void *arg)
{
	struct event_fd *entry = event_fd_create(fd, events, handler, arg);
	if (!entry)
		return -1;
	entry->once = 1;
	event_loop_add_internal(loop, entry);
	return 0;
}

static void event_loop_remove(struct event_loop *loop, struct event_fd *entry)
{
	log("Removing descriptor %d from event loop\n", entry->fd);

	SIMPLEQ_REMOVE(&loop->entries, entry, event_fd, entries);
	net_close(entry->fd);
	event_fd_destroy(entry);
	--loop->nfds;
}

static char *append_event(char *buf, size_t sz, char *ptr, const char *event)
{
	if (ptr > buf)
		ptr = stpecpy(ptr, buf + sz, ",");
	return stpecpy(ptr, buf + sz, event);
}

static char *events_to_string(short events)
{
	const size_t sz = 128;
	char *buf = calloc(1, sz);
	if (!buf)
		return NULL;

	char *ptr = buf;

	if (events & POLLNVAL)
		ptr = append_event(buf, sz, ptr, "POLLNVAL");
	if (events & POLLERR)
		ptr = append_event(buf, sz, ptr, "POLLERR");
	if (events & POLLHUP)
		ptr = append_event(buf, sz, ptr, "POLLHUP");
	if (events & POLLIN)
		ptr = append_event(buf, sz, ptr, "POLLIN");
	if (events & POLLOUT)
		ptr = append_event(buf, sz, ptr, "POLLOUT");

	return buf;
}

static struct pollfd *make_pollfds(const struct event_loop *loop)
{
	struct pollfd *fds = calloc(loop->nfds, sizeof(struct pollfd));
	if (!fds) {
		log_errno("calloc");
		return NULL;
	}

	struct event_fd *entry = SIMPLEQ_FIRST(&loop->entries);
	for (nfds_t i = 0; i < loop->nfds; ++i, entry = SIMPLEQ_NEXT(entry, entries)) {
		fds[i].fd = entry->fd;
		fds[i].events = entry->events;
	}

	log("Descriptors:\n");
	for (nfds_t i = 0; i < loop->nfds; ++i) {
		char *events = events_to_string(fds[i].events);
		log("    %d (%s)\n", fds[i].fd, events ? events : "");
		free(events);
	}

	return fds;
}

int event_loop_run(struct event_loop *loop)
{
	struct pollfd *fds = make_pollfds(loop);
	if (!fds)
		return -1;

	int ret = poll(fds, loop->nfds, -1);
	if (ret < 0) {
		log_errno("poll");
		return ret;
	}
	ret = 0;

	struct event_fd *entry = SIMPLEQ_FIRST(&loop->entries);
	for (nfds_t i = 0; i < loop->nfds; ++i) {
		struct event_fd *next = SIMPLEQ_NEXT(entry, entries);

		if (!fds[i].revents)
			goto next;

		char *events = events_to_string(fds[i].revents);
		log("Descriptor %d is ready: %s\n", fds[i].fd, events ? events : "");
		free(events);

		/* Execute all handlers but notice if any of them fail. */
		const int handler_ret = entry->handler(loop, fds[i].fd, fds[i].revents, entry->arg);
		if (handler_ret < 0)
			ret = handler_ret;

		if (entry->once)
			event_loop_remove(loop, entry);

	next:
		entry = next;
		continue;
	}

	free(fds);
	return ret;
}
