/*
 * Copyright (c) 2023 Egor Tensin <egor@tensin.name>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __JSON_H__
#define __JSON_H__

#include "log.h"

#include <json-c/json_object.h>

#include <stdint.h>

#define json_errno(fn)                                                                             \
	do {                                                                                       \
		log_err("JSON: %s failed\n", fn);                                                  \
	} while (0)

void json_free(struct json_object *);

const char *json_to_string(struct json_object *);
const char *json_to_string_pretty(struct json_object *);
struct json_object *json_from_string(const char *);

int json_clone(const struct json_object *, const char *key, struct json_object **value);

int json_send(struct json_object *, int fd);
struct json_object *json_recv(int fd);

int json_new_object(struct json_object **);
int json_new_array(struct json_object **);

int json_has(const struct json_object *, const char *key);

int json_get(const struct json_object *, const char *key, struct json_object **value);
int json_get_string(const struct json_object *, const char *key, const char **value);
int json_get_int(const struct json_object *, const char *key, int64_t *value);

int json_set(struct json_object *, const char *key, struct json_object *value);
int json_set_string(struct json_object *, const char *key, const char *value);
int json_set_int(struct json_object *, const char *key, int64_t value);

int json_set_const_key(struct json_object *, const char *, struct json_object *value);
int json_set_string_const_key(struct json_object *, const char *, const char *value);
int json_set_int_const_key(struct json_object *, const char *, int64_t value);

int json_append(struct json_object *arr, struct json_object *elem);

#endif
