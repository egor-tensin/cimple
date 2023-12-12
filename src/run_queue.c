/*
 * Copyright (c) 2022 Egor Tensin <egor@tensin.name>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "run_queue.h"
#include "json.h"
#include "log.h"

#include <json-c/json_object.h>

#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

struct run {
	int id;
	char *repo_url;
	char *repo_rev;
	int status;
	int exit_code;

	SIMPLEQ_ENTRY(run) entries;
};

int run_new(struct run **_entry, int id, const char *_repo_url, const char *_repo_rev,
            enum run_status status, int exit_code)
{
	struct run *entry = malloc(sizeof(struct run));
	if (!entry) {
		log_errno("malloc");
		goto fail;
	}

	char *repo_url = strdup(_repo_url);
	if (!repo_url) {
		log_errno("strdup");
		goto free_entry;
	}

	char *repo_rev = strdup(_repo_rev);
	if (!repo_rev) {
		log_errno("strdup");
		goto free_repo_url;
	}

	entry->id = id;
	entry->repo_url = repo_url;
	entry->repo_rev = repo_rev;
	entry->status = status;
	entry->exit_code = exit_code;

	*_entry = entry;
	return 0;

free_repo_url:
	free(repo_url);

free_entry:
	free(entry);

fail:
	return -1;
}

void run_destroy(struct run *entry)
{
	free(entry->repo_rev);
	free(entry->repo_url);
	free(entry);
}

int run_queued(struct run **entry, const char *repo_url, const char *repo_rev)
{
	return run_new(entry, -1, repo_url, repo_rev, RUN_STATUS_CREATED, -1);
}

int run_created(struct run **entry, int id, const char *repo_url, const char *repo_rev)
{
	return run_new(entry, id, repo_url, repo_rev, RUN_STATUS_CREATED, -1);
}

int run_to_json(const struct run *entry, struct json_object **_json)
{
	struct json_object *json = NULL;
	int ret = 0;

	ret = json_new_object(&json);
	if (ret < 0)
		return -1;
	ret = json_set_int_const_key(json, "id", entry->id);
	if (ret < 0)
		goto free;
	ret = json_set_int_const_key(json, "exit_code", entry->exit_code);
	if (ret < 0)
		goto free;
	ret = json_set_string_const_key(json, "repo_url", entry->repo_url);
	if (ret < 0)
		goto free;
	ret = json_set_string_const_key(json, "repo_rev", entry->repo_rev);
	if (ret < 0)
		goto free;

	*_json = json;
	return ret;

free:
	json_object_put(json);

	return ret;
}

int run_get_id(const struct run *entry)
{
	return entry->id;
}

const char *run_get_repo_url(const struct run *entry)
{
	return entry->repo_url;
}

const char *run_get_repo_rev(const struct run *entry)
{
	return entry->repo_rev;
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

int run_queue_to_json(const struct run_queue *queue, struct json_object **_json)
{
	struct json_object *json = NULL;
	int ret = 0;

	ret = json_new_array(&json);
	if (ret < 0)
		return ret;

	struct run *entry = NULL;
	SIMPLEQ_FOREACH(entry, queue, entries)
	{
		struct json_object *entry_json = NULL;
		ret = run_to_json(entry, &entry_json);
		if (ret < 0)
			goto free;

		ret = json_append(json, entry_json);
		if (ret < 0) {
			json_object_put(entry_json);
			goto free;
		}
	}

	*_json = json;
	return ret;

free:
	json_object_put(json);

	return ret;
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
