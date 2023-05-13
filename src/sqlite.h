/*
 * Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __SQLITE_H__
#define __SQLITE_H__

#include <sqlite3.h>

int sqlite_init(void);
void sqlite_destroy(void);

int sqlite_open(const char *path, sqlite3 **db, int flags);
int sqlite_open_rw(const char *path, sqlite3 **db);
int sqlite_open_ro(const char *path, sqlite3 **db);
void sqlite_close(sqlite3 *db);

int sqlite_exec(sqlite3 *db, const char *stmt, sqlite3_callback callback);
int sqlite_log_result(void *, int, char **, char **);

int sqlite_prepare(sqlite3 *db, const char *stmt, sqlite3_stmt **result);
void sqlite_finalize(sqlite3_stmt *);
int sqlite_step(sqlite3_stmt *);

int sqlite_column_int(sqlite3_stmt *, int column_index);
int sqlite_column_text(sqlite3_stmt *, int column_index, char **result);
int sqlite_column_blob(sqlite3_stmt *, int column_index, unsigned char **result);

int sqlite_exec_as_transaction(sqlite3 *db, const char *stmt);

int sqlite_get_user_version(sqlite3 *db, unsigned int *version);
int sqlite_set_foreign_keys(sqlite3 *db);

#endif
