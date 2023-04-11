/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "sqlite.h"
#include "compiler.h"
#include "log.h"

#include <sqlite3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define sqlite_errno(var, fn)                                                                      \
	do {                                                                                       \
		log_err("%s: %s\n", fn, sqlite3_errstr(var));                                      \
		var = -var;                                                                        \
	} while (0)

#define sqlite_errno_if(expr, fn)                                                                  \
	do {                                                                                       \
		int CONCAT(ret, __LINE__) = expr;                                                  \
		if (CONCAT(ret, __LINE__))                                                         \
			sqlite_errno(CONCAT(ret, __LINE__), fn);                                   \
	} while (0)

int sqlite_init()
{
	int ret = 0;

	ret = sqlite3_initialize();
	if (ret) {
		sqlite_errno(ret, "sqlite3_initialize");
		return ret;
	}

	return ret;
}

void sqlite_destroy()
{
	sqlite_errno_if(sqlite3_shutdown(), "sqlite3_shutdown");
}

int sqlite_open(const char *path, sqlite3 **db, int flags)
{
	int ret = 0;

	ret = sqlite3_open_v2(path, db, flags, NULL);
	if (ret) {
		sqlite_errno(ret, "sqlite3_open_v2");
		return ret;
	}

	return ret;
}

int sqlite_open_rw(const char *path, sqlite3 **db)
{
	static const int flags = SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE;
	return sqlite_open(path, db, flags);
}

int sqlite_open_ro(const char *path, sqlite3 **db)
{
	static const int flags = SQLITE_OPEN_READONLY;
	return sqlite_open(path, db, flags);
}

void sqlite_close(sqlite3 *db)
{
	sqlite_errno_if(sqlite3_close(db), "sqlite3_close");
}

int sqlite_exec(sqlite3 *db, const char *stmt, sqlite3_callback callback)
{
	int ret = 0;

	ret = sqlite3_exec(db, stmt, callback, NULL, NULL);
	if (ret) {
		sqlite_errno(ret, "sqlite3_exec");
		return ret;
	}

	return ret;
}

int sqlite_log_result(UNUSED void *arg, int numof_columns, char **values, char **column_names)
{
	log("Row:\n");
	for (int i = 0; i < numof_columns; ++i) {
		log("\t%s: %s\n", column_names[i], values[i]);
	}
	return 0;
}

int sqlite_prepare(sqlite3 *db, const char *stmt, sqlite3_stmt **result)
{
	int ret = 0;

	ret = sqlite3_prepare_v2(db, stmt, -1, result, NULL);
	if (ret) {
		sqlite_errno(ret, "sqlite3_prepare_v2");
		return ret;
	}

	return ret;
}

void sqlite_finalize(sqlite3_stmt *stmt)
{
	sqlite_errno_if(sqlite3_finalize(stmt), "sqlite3_finalize");
}

int sqlite_step(sqlite3_stmt *stmt)
{
	int ret = 0;

	ret = sqlite3_step(stmt);

	switch (ret) {
	case SQLITE_ROW:
	case SQLITE_DONE:
		return 0;

	default:
		sqlite_errno(ret, "sqlite3_step");
		return ret;
	}
}

int sqlite_column_int(sqlite3_stmt *stmt, int index)
{
	return sqlite3_column_int(stmt, index);
}

int sqlite_column_text(sqlite3_stmt *stmt, int index, char **result)
{
	const unsigned char *value;
	size_t nb;
	int ret = 0;

	value = sqlite3_column_text(stmt, index);
	if (!value) {
		ret = sqlite3_errcode(sqlite3_db_handle(stmt));
		if (ret) {
			sqlite_errno(ret, "sqlite3_column_text");
			return ret;
		}

		*result = NULL;
		return 0;
	}

	ret = sqlite3_column_bytes(stmt, index);
	nb = (size_t)ret;

	*result = calloc(nb + 1, 1);
	if (!*result) {
		log_errno("calloc");
		return -1;
	}

	memcpy(*result, value, nb);
	return 0;
}

int sqlite_column_blob(sqlite3_stmt *stmt, int index, unsigned char **result)
{
	const unsigned char *value;
	size_t nb;
	int ret = 0;

	value = sqlite3_column_blob(stmt, index);
	if (!value) {
		ret = sqlite3_errcode(sqlite3_db_handle(stmt));
		if (ret) {
			sqlite_errno(ret, "sqlite3_column_text");
			return ret;
		}

		*result = NULL;
		return 0;
	}

	ret = sqlite3_column_bytes(stmt, index);
	nb = (size_t)ret;

	*result = malloc(nb);
	if (!*result) {
		log_errno("malloc");
		return -1;
	}

	memcpy(*result, value, nb);
	return 0;
}

int sqlite_exec_as_transaction(sqlite3 *db, const char *stmt)
{
	static const char *const FMT = "BEGIN; %s COMMIT;";

	char *full_stmt;
	size_t nb;
	int ret = 0;

	ret = snprintf(NULL, 0, FMT, stmt);
	nb = (size_t)ret + 1;
	ret = 0;

	full_stmt = malloc(nb);
	if (!full_stmt) {
		log_errno("malloc");
		return -1;
	}
	snprintf(full_stmt, nb, FMT, stmt);

	ret = sqlite_exec(db, stmt, NULL);
	goto free;

free:
	free(full_stmt);

	return ret;
}

int sqlite_get_user_version(sqlite3 *db, unsigned int *version)
{
	sqlite3_stmt *stmt;
	int result, ret = 0;

	ret = sqlite_prepare(db, "PRAGMA user_version;", &stmt);
	if (ret < 0)
		return ret;
	ret = sqlite_step(stmt);
	if (ret < 0)
		goto finalize;

	result = sqlite_column_int(stmt, 0);
	if (result < 0) {
		log_err("Invalid database version: %d\n", result);
		return -1;
	}
	*version = (unsigned int)result;

	goto finalize;

finalize:
	sqlite_finalize(stmt);

	return ret;
}

int sqlite_set_foreign_keys(sqlite3 *db)
{
	return sqlite_exec(db, "PRAGMA foreign_keys = ON;", NULL);
}
