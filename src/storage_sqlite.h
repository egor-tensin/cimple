/*
 * Copyright (c) 2022 Egor Tensin <egor@tensin.name>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __STORAGE_SQLITE_H__
#define __STORAGE_SQLITE_H__

#include "process.h"
#include "run_queue.h"

struct storage_settings;
struct storage_sqlite_setttings;

struct storage;
struct storage_sqlite;

int storage_sqlite_settings_create(struct storage_settings *, const char *path);
void storage_sqlite_settings_destroy(const struct storage_settings *);

int storage_sqlite_create(struct storage *, const struct storage_settings *);
void storage_sqlite_destroy(struct storage *);

int storage_sqlite_run_create(struct storage *, const char *repo_url, const char *rev);
int storage_sqlite_run_finished(struct storage *, int id, const struct process_output *);

int storage_sqlite_get_runs(struct storage *, struct run_queue *runs);
int storage_sqlite_get_run_queue(struct storage *, struct run_queue *runs);

#endif
