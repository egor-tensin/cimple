/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "tcp_server.h"
#include "compiler.h"
#include "event_loop.h"
#include "file.h"
#include "log.h"
#include "net.h"
#include "signal.h"

#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <sys/queue.h>
#include <unistd.h>

/*
 * This is a simple threaded TCP server implementation. Each client is handled
 * in a separate thread.
 *
 * It used to be much simpler; basically, we have two types of client
 * connections: those made by cimple-worker and cimple-client respectively.
 * cimple-server would keep track of cimple-worker threads/connections, and
 * clean them up when assigning tasks/on shutdown.
 *
 * What about cimple-client connections though? I struggled to come up with a
 * scheme that would allow cimple-server to clean them up gracefully. When
 * would it do the cleanup even? I didn't want to do it on shutdown, since
 * there would be potentially a lot of them.
 *
 * One solution is to make client threads detached. This is a common advise;
 * I really don't understand the merit of this approach though. Client threads
 * actively work on shared data, take locks, etc. Data corruption is very
 * likely after the main thread exits and all the rest are killed.
 *
 * Another approach is pre-threading; we make a number of threads beforehand
 * and handle all client connections; I view this approach as limiting in
 * principle; probably that's foolish of me.
 *
 * Finally, I cannot bring myself to do non-blocking I/O. I honestly fear the
 * amount of work it would require to maintain read buffers, etc.
 *
 * So I came up with this convoluted scheme. The TCP server adds the listening
 * socket to the event loop, as before. Each client thread makes an eventfd
 * descriptor that it writes to when it's about to finish. The eventfd
 * descriptor is added to the event loop; once it's readable, we clean up the
 * client thread quickly from the main event loop thread. The TCP server itself
 * keeps track of client threads; on shutdown, it cleans up those still working.
 *
 * I'm _really_ not sure about this approach, it seems fishy as hell; I guess,
 * we'll see.
 */

struct client {
	struct tcp_server *server;
	int conn_fd;

	int cleanup_fd;

	pid_t tid;
	pthread_t thread;

	SIMPLEQ_ENTRY(client) entries;
};

SIMPLEQ_HEAD(client_queue, client);

struct tcp_server {
	struct event_loop *loop;

	tcp_server_conn_handler conn_handler;
	void *conn_handler_arg;

	struct client_queue client_queue;

	int accept_fd;
};

static void client_destroy(struct client *client)
{
	log_debug("Cleaning up client thread %d\n", client->tid);

	SIMPLEQ_REMOVE(&client->server->client_queue, client, client, entries);
	pthread_errno_if(pthread_join(client->thread, NULL), "pthread_join");
	file_close(client->cleanup_fd);
	net_close(client->conn_fd);
	free(client);
}

static int client_destroy_handler(UNUSED struct event_loop *loop, UNUSED int fd,
                                  UNUSED short revents, void *_client)
{
	struct client *client = (struct client *)_client;
	log_debug("Client thread %d indicated that it's done\n", client->tid);

	client_destroy(client);
	return 0;
}

static void *client_thread_func(void *_client)
{
	struct client *client = (struct client *)_client;
	int ret = 0;

	client->tid = gettid();
	log_debug("New client thread thread %d has started\n", client->tid);

	/* Let the client thread handle its signals except those that should be
	 * handled in the main thread. */
	ret = signal_block_sigterms();
	if (ret < 0)
		goto cleanup;

	ret = client->server->conn_handler(client->conn_fd, client->server->conn_handler_arg);
	if (ret < 0)
		goto cleanup;

cleanup:
	log_errno_if(eventfd_write(client->cleanup_fd, 1), "eventfd_write");

	return NULL;
}

static int client_create_thread(struct client *client)
{
	sigset_t old_mask;
	int ret = 0;

	/* Block all signals (we'll unblock them later); the client thread will
	 * have all signals blocked initially. This allows the main thread to
	 * handle SIGINT/SIGTERM/etc. */
	ret = signal_block_all(&old_mask);
	if (ret < 0)
		return ret;

	ret = pthread_create(&client->thread, NULL, client_thread_func, client);
	if (ret) {
		pthread_errno(ret, "pthread_create");
		goto restore_mask;
	}

restore_mask:
	/* Restore the previously-enabled signals for handling in the main thread. */
	signal_set_mask(&old_mask);

	return ret;
}

static int client_create(struct tcp_server *server, int conn_fd)
{
	int ret = 0;

	struct client *client = calloc(1, sizeof(struct client));
	if (!client) {
		log_errno("calloc");
		return -1;
	}

	client->server = server;
	client->conn_fd = conn_fd;

	ret = eventfd(0, EFD_CLOEXEC);
	if (ret < 0) {
		log_errno("eventfd");
		goto free;
	}
	client->cleanup_fd = ret;

	ret = event_loop_add_once(server->loop, client->cleanup_fd, POLLIN, client_destroy_handler,
	                          client);
	if (ret < 0)
		goto close_cleanup_fd;

	SIMPLEQ_INSERT_TAIL(&server->client_queue, client, entries);

	ret = client_create_thread(client);
	if (ret < 0)
		goto remove_from_client_queue;

	return ret;

remove_from_client_queue:
	SIMPLEQ_REMOVE(&server->client_queue, client, client, entries);

close_cleanup_fd:
	file_close(client->cleanup_fd);

free:
	free(client);

	return ret;
}

static void client_queue_create(struct client_queue *client_queue)
{
	SIMPLEQ_INIT(client_queue);
}

static void client_queue_destroy(struct client_queue *client_queue)
{
	struct client *entry1 = SIMPLEQ_FIRST(client_queue);
	while (entry1) {
		struct client *entry2 = SIMPLEQ_NEXT(entry1, entries);
		client_destroy(entry1);
		entry1 = entry2;
	}
}

static int tcp_server_accept_handler(UNUSED struct event_loop *loop, UNUSED int fd,
                                     UNUSED short revents, void *_server)
{
	struct tcp_server *server = (struct tcp_server *)_server;
	return tcp_server_accept(server);
}

int tcp_server_create(struct tcp_server **_server, struct event_loop *loop, const char *port,
                      tcp_server_conn_handler conn_handler, void *conn_handler_arg)
{
	int ret = 0;

	struct tcp_server *server = calloc(1, sizeof(struct tcp_server));
	if (!server) {
		log_errno("calloc");
		return -1;
	}

	server->loop = loop;

	server->conn_handler = conn_handler;
	server->conn_handler_arg = conn_handler_arg;

	client_queue_create(&server->client_queue);

	ret = net_bind(port);
	if (ret < 0)
		goto free;
	server->accept_fd = ret;

	ret = event_loop_add(loop, server->accept_fd, POLLIN, tcp_server_accept_handler, server);
	if (ret < 0)
		goto close;

	*_server = server;
	return ret;

close:
	net_close(server->accept_fd);
free:
	free(server);

	return ret;
}

void tcp_server_destroy(struct tcp_server *server)
{
	net_close(server->accept_fd);
	client_queue_destroy(&server->client_queue);
	free(server);
}

int tcp_server_accept(struct tcp_server *server)
{
	int conn_fd = -1, ret = 0;

	ret = net_accept(server->accept_fd);
	if (ret < 0)
		return ret;
	conn_fd = ret;

	ret = client_create(server, conn_fd);
	if (ret < 0)
		goto close_conn;

	return ret;

close_conn:
	net_close(conn_fd);

	return ret;
}
