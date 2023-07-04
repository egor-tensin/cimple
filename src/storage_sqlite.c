/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "storage_sqlite.h"
#include "log.h"
#include "run_queue.h"
#include "sql/sqlite_sql.h"
#include "sqlite.h"
#include "storage.h"

#include <sqlite3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct storage_sqlite_settings {
	char *path;
};

int storage_sqlite_settings_create(struct storage_settings *settings, const char *path)
{
	struct storage_sqlite_settings *sqlite = malloc(sizeof(struct storage_sqlite_settings));
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

void storage_sqlite_settings_destroy(const struct storage_settings *settings)
{
	free(settings->sqlite->path);
	free(settings->sqlite);
}

struct storage_sqlite {
	sqlite3 *db;

	sqlite3_stmt *stmt_repo_find;
	sqlite3_stmt *stmt_repo_insert;
	sqlite3_stmt *stmt_run_insert;
	sqlite3_stmt *stmt_run_finished;
};

static int storage_sqlite_upgrade_to(struct storage_sqlite *storage, size_t version)
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

static int storage_sqlite_upgrade_from_to(struct storage_sqlite *storage, size_t from, size_t to)
{
	int ret = 0;

	for (size_t i = from; i < to; ++i) {
		log("Upgrading SQLite database from version %zu to version %zu\n", i, i + 1);
		ret = storage_sqlite_upgrade_to(storage, i);
		if (ret < 0) {
			log_err("Failed to upgrade to version %zu\n", i + 1);
			return ret;
		}
	}

	return ret;
}

static int storage_sqlite_upgrade(struct storage_sqlite *storage)
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

	return storage_sqlite_upgrade_from_to(storage, current_version, newest_version);
}

static int storage_sqlite_setup(struct storage_sqlite *storage)
{
	int ret = 0;

	ret = sqlite_set_foreign_keys(storage->db);
	if (ret < 0)
		return ret;

	ret = storage_sqlite_upgrade(storage);
	if (ret < 0)
		return ret;

	return ret;
}

static int storage_sqlite_prepare_statements(struct storage_sqlite *storage)
{
	/* clang-format off */
	static const char *const fmt_repo_find    = "SELECT id FROM cimple_repos WHERE url = ?;";
	static const char *const fmt_repo_insert  = "INSERT INTO cimple_repos(url) VALUES (?) RETURNING id;";
	static const char *const fmt_run_insert   = "INSERT INTO cimple_runs(status, ec, output, repo_id, rev) VALUES (1, -1, x'', ?, ?) RETURNING id;";
	static const char *const fmt_run_finished = "UPDATE cimple_runs SET status = 2, ec = ? WHERE id = ?;";
	/* clang-format on */

	int ret = 0;

	ret = sqlite_prepare(storage->db, fmt_repo_find, &storage->stmt_repo_find);
	if (ret < 0)
		return ret;
	ret = sqlite_prepare(storage->db, fmt_repo_insert, &storage->stmt_repo_insert);
	if (ret < 0)
		goto finalize_repo_find;
	ret = sqlite_prepare(storage->db, fmt_run_insert, &storage->stmt_run_insert);
	if (ret < 0)
		goto finalize_repo_insert;
	ret = sqlite_prepare(storage->db, fmt_run_finished, &storage->stmt_run_finished);
	if (ret < 0)
		goto finalize_run_insert;

	return ret;

finalize_run_insert:
	sqlite_finalize(storage->stmt_run_insert);
finalize_repo_insert:
	sqlite_finalize(storage->stmt_repo_insert);
finalize_repo_find:
	sqlite_finalize(storage->stmt_repo_find);

	return ret;
}

static void storage_sqlite_finalize_statements(struct storage_sqlite *storage)
{
	sqlite_finalize(storage->stmt_run_finished);
	sqlite_finalize(storage->stmt_run_insert);
	sqlite_finalize(storage->stmt_repo_insert);
	sqlite_finalize(storage->stmt_repo_find);
}

int storage_sqlite_create(struct storage *storage, const struct storage_settings *settings)
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
	ret = storage_sqlite_setup(sqlite);
	if (ret < 0)
		goto close;
	ret = storage_sqlite_prepare_statements(sqlite);
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

void storage_sqlite_destroy(struct storage *storage)
{
	storage_sqlite_finalize_statements(storage->sqlite);
	sqlite_close(storage->sqlite->db);
	sqlite_destroy();
	free(storage->sqlite);
}

static int storage_sqlite_find_repo(struct storage_sqlite *storage, const char *url)
{
	sqlite3_stmt *stmt = storage->stmt_repo_find;
	int ret = 0;

	ret = sqlite_bind_text(stmt, 1, url);
	if (ret < 0)
		goto reset;
	ret = sqlite_step(stmt);
	if (ret < 0)
		goto reset;

	if (!ret)
		goto reset;

	ret = sqlite_column_int(stmt, 0);
	goto reset;

reset:
	sqlite_reset(stmt);

	return ret;
}

static int storage_sqlite_insert_repo(struct storage_sqlite *storage, const char *url)
{
	sqlite3_stmt *stmt = storage->stmt_repo_insert;
	int ret = 0;

	ret = storage_sqlite_find_repo(storage, url);
	if (ret < 0)
		return ret;

	if (ret)
		return ret;

	ret = sqlite_bind_text(stmt, 1, url);
	if (ret < 0)
		goto reset;
	ret = sqlite_step(stmt);
	if (ret < 0)
		goto reset;

	if (!ret) {
		ret = -1;
		log_err("Failed to insert a repository\n");
		goto reset;
	}

	ret = sqlite_column_int(stmt, 0);
	goto reset;

reset:
	sqlite_reset(stmt);

	return ret;
}

static int storage_sqlite_insert_run(struct storage_sqlite *storage, int repo_id, const char *rev)
{
	sqlite3_stmt *stmt = storage->stmt_run_insert;
	int ret = 0;

	ret = sqlite_bind_int(stmt, 1, repo_id);
	if (ret < 0)
		goto reset;
	ret = sqlite_bind_text(stmt, 2, rev);
	if (ret < 0)
		goto reset;
	ret = sqlite_step(stmt);
	if (ret < 0)
		goto reset;

	if (!ret) {
		ret = -1;
		log_err("Failed to insert a run\n");
		goto reset;
	}

	ret = sqlite_column_int(stmt, 0);
	goto reset;

reset:
	sqlite_reset(stmt);

	return ret;
}

int storage_sqlite_run_create(struct storage *storage, const char *repo_url, const char *rev)
{
	int ret = 0;

	ret = storage_sqlite_insert_repo(storage->sqlite, repo_url);
	if (ret < 0)
		return ret;

	ret = storage_sqlite_insert_run(storage->sqlite, ret, rev);
	if (ret < 0)
		return ret;

	return ret;
}

int storage_sqlite_run_finished(struct storage *storage, int run_id, int ec)
{
	sqlite3_stmt *stmt = storage->sqlite->stmt_run_finished;
	int ret = 0;

	ret = sqlite_bind_int(stmt, 1, ec);
	if (ret < 0)
		goto reset;
	ret = sqlite_bind_int(stmt, 2, run_id);
	if (ret < 0)
		goto reset;
	ret = sqlite_step(stmt);
	if (ret < 0)
		goto reset;

reset:
	sqlite_reset(stmt);

	return ret;
}

static int storage_sqlite_row_to_run(struct sqlite3_stmt *stmt, struct run **run)
{
	int ret = 0;

	int id = sqlite_column_int(stmt, 0);

	char *url = NULL;
	ret = sqlite_column_text(stmt, 1, &url);
	if (ret < 0)
		return ret;

	char *rev = NULL;
	ret = sqlite_column_text(stmt, 2, &rev);
	if (ret < 0)
		goto free_url;

	ret = run_create(run, id, url, rev);
	if (ret < 0)
		goto free_rev;

	log("Adding a run %d for repository %s to the queue\n", id, url);

free_rev:
	free(rev);

free_url:
	free(url);

	return ret;
}

int storage_sqlite_get_run_queue(struct storage *storage, struct run_queue *queue)
{
	/* clang-format off */
	static const char *const fmt = "SELECT cimple_runs.id, cimple_repos.url, cimple_runs.rev FROM cimple_runs INNER JOIN cimple_repos ON cimple_runs.repo_id = cimple_repos.id WHERE cimple_runs.status = 1;";
	/* clang-format on */

	sqlite3_stmt *stmt;
	int ret = 0;

	ret = sqlite_prepare(storage->sqlite->db, fmt, &stmt);
	if (ret < 0)
		return ret;

	while (1) {
		ret = sqlite_step(stmt);
		if (!ret)
			break;
		if (ret < 0)
			goto run_queue_destroy;

		struct run *run = NULL;

		ret = storage_sqlite_row_to_run(stmt, &run);
		if (ret < 0)
			goto run_queue_destroy;

		run_queue_add_last(queue, run);
	}

	goto finalize;

run_queue_destroy:
	run_queue_destroy(queue);

finalize:
	sqlite_finalize(stmt);

	return ret;
}
