/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "storage.h"
#include "log.h"
#include "storage_sqlite.h"

#include <stddef.h>

typedef void (*storage_settings_destroy_t)(const struct storage_settings *);
typedef int (*storage_create_t)(struct storage *, const struct storage_settings *);
typedef void (*storage_destroy_t)(struct storage *);

struct storage_api {
	storage_settings_destroy_t destroy_settings;
	storage_create_t create;
	storage_destroy_t destroy;
};

static const struct storage_api apis[] = {
    {
	storage_sqlite_settings_destroy,
	storage_sqlite_create,
	storage_sqlite_destroy,
    },
};

static size_t numof_apis(void)
{
	return sizeof(apis) / sizeof(apis[0]);
}

static const struct storage_api *get_api(enum storage_type type)
{
	if (type < 0)
		goto invalid_type;
	if ((size_t)type > numof_apis())
		goto invalid_type;

	return &apis[type];

invalid_type:
	log_err("Unsupported storage type: %d\n", type);
	return NULL;
}

void storage_settings_destroy(const struct storage_settings *settings)
{
	const struct storage_api *api = get_api(settings->type);
	if (!api)
		return;
	api->destroy_settings(settings);
}

int storage_create(struct storage *storage, const struct storage_settings *settings)
{
	int ret = 0;
	const struct storage_api *api = get_api(settings->type);
	if (!api)
		return -1;
	ret = api->create(storage, settings);
	if (ret < 0)
		return ret;
	storage->type = settings->type;
	return ret;
}

void storage_destroy(struct storage *storage)
{
	const struct storage_api *api = get_api(storage->type);
	if (!api)
		return;
	api->destroy(storage);
}
