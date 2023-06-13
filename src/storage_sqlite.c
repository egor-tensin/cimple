/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "storage_sqlite.h"
#include "log.h"
#include "sql/sqlite_sql.h"
#include "sqlite.h"
#include "storage.h"

#include <sqlite3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct storage_settings_sqlite {
	char *path;
};

int storage_settings_create_sqlite(struct storage_settings *settings, const char *path)
{
	struct storage_settings_sqlite *sqlite = malloc(sizeof(struct storage_settings_sqlite));
	if (!sqlite) {
		log_errno("malloc");
		return -1;
	}

	sqlite->path = strdup(path);
	if (!sqlite->path) {
		log_errno("strdup");
		goto free;
	}

	settings->type = STORAGE_TYPE_SQLITE;
	settings->sqlite = sqlite;
	return 0;

free:
	free(sqlite);

	return -1;
}

void storage_settings_destroy_sqlite(const struct storage_settings *settings)
{
	free(settings->sqlite->path);
	free(settings->sqlite);
}

struct storage_sqlite {
	sqlite3 *db;
};

static int storage_upgrade_sqlite_to(struct storage_sqlite *storage, size_t version)
{
	static const char *const fmt = "%s PRAGMA user_version = %zu;";

	const char *script = sql_sqlite_files[version];
	int ret = 0;

	ret = snprintf(NULL, 0, fmt, script, version + 1);
	size_t nb = (size_t)ret + 1;
	ret = 0;

	char *full_script = malloc(nb);
	if (!full_script) {
		log_errno("malloc");
		return -1;
	}
	snprintf(full_script, nb, fmt, script, version + 1);

	ret = sqlite_exec_as_transaction(storage->db, full_script);
	goto free;

free:
	free(full_script);

	return ret;
}

static int storage_upgrade_sqlite_from_to(struct storage_sqlite *storage, size_t from, size_t to)
{
	int ret = 0;

	for (size_t i = from; i < to; ++i) {
		log("Upgrading SQLite database from version %zu to version %zu\n", i, i + 1);
		ret = storage_upgrade_sqlite_to(storage, i);
		if (ret < 0) {
			log_err("Failed to upgrade to version %zu\n", i + 1);
			return ret;
		}
	}

	return ret;
}

static int storage_upgrade_sqlite(struct storage_sqlite *storage)
{
	unsigned int current_version = 0;
	int ret = 0;

	ret = sqlite_get_user_version(storage->db, &current_version);
	if (ret < 0)
		return ret;
	log("SQLite database version: %u\n", current_version);

	size_t newest_version = sizeof(sql_sqlite_files) / sizeof(sql_sqlite_files[0]);
	log("Newest database version: %zu\n", newest_version);

	if (current_version > newest_version) {
		log_err("Unknown database version: %u\n", current_version);
		return -1;
	}

	if (current_version == newest_version) {
		log("SQLite database already at the newest version\n");
		return 0;
	}

	return storage_upgrade_sqlite_from_to(storage, current_version, newest_version);
}

static int storage_prepare_sqlite(struct storage_sqlite *storage)
{
	int ret = 0;

	ret = sqlite_set_foreign_keys(storage->db);
	if (ret < 0)
		return ret;

	ret = storage_upgrade_sqlite(storage);
	if (ret < 0)
		return ret;

	return ret;
}

int storage_create_sqlite(struct storage *storage, const struct storage_settings *settings)
{
	int ret = 0;

	log("Using SQLite database at %s\n", settings->sqlite->path);

	struct storage_sqlite *sqlite = malloc(sizeof(struct storage_sqlite));
	if (!sqlite) {
		log_errno("malloc");
		return -1;
	}

	ret = sqlite_init();
	if (ret < 0)
		goto free;
	ret = sqlite_open_rw(settings->sqlite->path, &sqlite->db);
	if (ret < 0)
		goto destroy;
	ret = storage_prepare_sqlite(sqlite);
	if (ret < 0)
		goto close;

	storage->sqlite = sqlite;
	return ret;

close:
	sqlite_close(storage->sqlite->db);
destroy:
	sqlite_destroy();
free:
	free(sqlite);

	return ret;
}

void storage_destroy_sqlite(struct storage *storage)
{
	sqlite_close(storage->sqlite->db);
	sqlite_destroy();
	free(storage->sqlite);
}
