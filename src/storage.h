/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __STORAGE_H__
#define __STORAGE_H__

#include "storage_sqlite.h"

enum storage_type {
	STORAGE_TYPE_SQLITE,
};

struct storage_settings {
	enum storage_type type;
	union {
		struct storage_settings_sqlite sqlite;
	};
};

void storage_settings_destroy(const struct storage_settings *);

struct storage {
	enum storage_type type;
	union {
		struct storage_sqlite *sqlite;
	};
};

int storage_create(struct storage *, const struct storage_settings *);
void storage_destroy(struct storage *);

#endif
