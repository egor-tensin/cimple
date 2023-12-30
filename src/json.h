/*
 * Copyright (c) 2023 Egor Tensin <egor@tensin.name>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __JSON_H__
#define __JSON_H__

#include <json-c/json_object.h>

#include <stdint.h>

void libjson_free(struct json_object *);

const char *libjson_to_string(struct json_object *);
const char *libjson_to_string_pretty(struct json_object *);
struct json_object *libjson_from_string(const char *);

int libjson_clone(const struct json_object *, const char *key, struct json_object **value);

int libjson_send(struct json_object *, int fd);
struct json_object *libjson_recv(int fd);

int libjson_new_object(struct json_object **);
int libjson_new_array(struct json_object **);

int libjson_has(const struct json_object *, const char *key);

int libjson_get(const struct json_object *, const char *key, struct json_object **value);
int libjson_get_string(const struct json_object *, const char *key, const char **value);
int libjson_get_int(const struct json_object *, const char *key, int64_t *value);

int libjson_set(struct json_object *, const char *key, struct json_object *value);
int libjson_set_string(struct json_object *, const char *key, const char *value);
int libjson_set_int(struct json_object *, const char *key, int64_t value);

int libjson_set_const_key(struct json_object *, const char *, struct json_object *value);
int libjson_set_string_const_key(struct json_object *, const char *, const char *value);
int libjson_set_int_const_key(struct json_object *, const char *, int64_t value);

int libjson_append(struct json_object *arr, struct json_object *elem);

#endif
