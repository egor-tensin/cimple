/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "storage_sqlite.h"
#include "log.h"
#include "storage.h"

#include <sqlite3.h>

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

int storage_settings_create_sqlite(struct storage_settings *settings, const char *path)
{
	settings->sqlite.path = strdup(path);
	if (!settings->sqlite.path) {
		log_errno("strdup");
		return -1;
	}

	settings->type = STORAGE_TYPE_SQLITE;
	return 0;
}

void storage_settings_destroy_sqlite(const struct storage_settings *settings)
{
	free(settings->sqlite.path);
}

struct storage_sqlite {
	sqlite3 *db;
};

int storage_create_sqlite(struct storage *storage, const struct storage_settings *settings)
{
	int ret = 0;

	storage->sqlite = malloc(sizeof(storage->sqlite));
	if (!storage->sqlite) {
		log_errno("malloc");
		return -1;
	}

	ret = sqlite3_open(settings->sqlite.path, &storage->sqlite->db);
	if (ret) {
		sqlite_errno(ret, "sqlite3_open");
		return ret;
	}

	return 0;
}

void storage_destroy_sqlite(struct storage *storage)
{
	sqlite_errno_if(sqlite3_close(storage->sqlite->db), "sqlite3_close");
	free(storage->sqlite);
}
