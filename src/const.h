/*
 * Copyright (c) 2022 Egor Tensin <egor@tensin.name>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __CONST_H__
#define __CONST_H__

extern const char *project_version;
extern const char *project_rev;

extern const char *default_host;
extern const char *default_port;
extern const char *default_sqlite_path;

#define CMD_QUEUE_RUN    "queue-run"
#define CMD_NEW_WORKER   "new-worker"
#define CMD_START_RUN    "start-run"
#define CMD_FINISHED_RUN "finished-run"
#define CMD_GET_RUNS     "get-runs"

#endif
