/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __STORAGE_SQLITE_H__
#define __STORAGE_SQLITE_H__

struct storage_settings;

struct storage_settings_sqlite {
	char *path;
};

struct storage;
struct storage_sqlite;

int storage_settings_create_sqlite(struct storage_settings *, const char *path);
void storage_settings_destroy_sqlite(const struct storage_settings *);

int storage_create_sqlite(struct storage *, const struct storage_settings *);
void storage_destroy_sqlite(struct storage *);

#endif
