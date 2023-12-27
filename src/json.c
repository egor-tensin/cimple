/*
 * Copyright (c) 2023 Egor Tensin <egor@tensin.name>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "json.h"
#include "buf.h"
#include "log.h"
#include "net.h"

#include <json-c/json_object.h>
#include <json-c/json_tokener.h>

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void json_free(struct json_object *obj)
{
	json_object_put(obj);
}

static const char *json_to_string_internal(struct json_object *obj, int flags)
{
	const char *result = json_object_to_json_string_ext(obj, flags);
	if (!result) {
		json_errno("json_object_to_json_string");
		return NULL;
	}
	return result;
}

const char *json_to_string(struct json_object *obj)
{
	return json_to_string_internal(obj, JSON_C_TO_STRING_SPACED);
}

const char *json_to_string_pretty(struct json_object *obj)
{
	return json_to_string_internal(obj, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY);
}

struct json_object *json_from_string(const char *src)
{
	enum json_tokener_error error;

	struct json_object *result = json_tokener_parse_verbose(src, &error);
	if (!result) {
		json_errno("json_tokener_parse_verbose");
		log_err("JSON: parsing failed: %s\n", json_tokener_error_desc(error));
		return NULL;
	}

	return result;
}

int json_clone(const struct json_object *obj, const char *key, struct json_object **_value)
{
	int ret = 0;

	struct json_object *old_value = NULL;
	ret = json_get(obj, key, &old_value);
	if (ret < 0)
		return ret;

	struct json_object *new_value = NULL;
	ret = json_object_deep_copy(old_value, &new_value, NULL);
	if (ret < 0)
		return ret;

	*_value = new_value;
	return ret;
}

int json_send(struct json_object *obj, int fd)
{
	int ret = 0;

	const char *str = json_to_string(obj);
	if (!str)
		return -1;

	struct buf *buf = NULL;
	ret = buf_create_from_string(&buf, str);
	if (ret < 0)
		return ret;

	ret = net_send_buf(fd, buf);
	buf_destroy(buf);
	return ret;
}

struct json_object *json_recv(int fd)
{
	struct json_object *result = NULL;
	int ret = 0;

	struct buf *buf = NULL;
	ret = net_recv_buf(fd, &buf);
	if (ret < 0)
		return NULL;

	result = json_from_string((const char *)buf_get_data(buf));
	if (!result)
		goto destroy_buf;

destroy_buf:
	free((void *)buf_get_data(buf));
	buf_destroy(buf);

	return result;
}

int json_new_object(struct json_object **_obj)
{
	struct json_object *obj = json_object_new_object();
	if (!obj) {
		json_errno("json_object_new_object");
		return -1;
	}
	*_obj = obj;
	return 0;
}

int json_new_array(struct json_object **_arr)
{
	struct json_object *arr = json_object_new_array();
	if (!arr) {
		json_errno("json_object_new_array");
		return -1;
	}
	*_arr = arr;
	return 0;
}

int json_has(const struct json_object *obj, const char *key)
{
	return json_object_object_get_ex(obj, key, NULL);
}

int json_get(const struct json_object *obj, const char *key, struct json_object **value)
{
	if (!json_has(obj, key)) {
		log_err("JSON: key is missing: %s\n", key);
		return -1;
	}

	return json_object_object_get_ex(obj, key, value);
}

int json_get_string(const struct json_object *obj, const char *key, const char **_value)
{
	struct json_object *value = NULL;

	int ret = json_get(obj, key, &value);
	if (ret < 0)
		return ret;

	if (!json_object_is_type(value, json_type_string)) {
		log_err("JSON: key is not a string: %s\n", key);
		return -1;
	}

	*_value = json_object_get_string(value);
	return 0;
}

int json_get_int(const struct json_object *obj, const char *key, int64_t *_value)
{
	struct json_object *value = NULL;

	int ret = json_get(obj, key, &value);
	if (ret < 0)
		return ret;

	if (!json_object_is_type(value, json_type_int)) {
		log_err("JSON: key is not an integer: %s\n", key);
		return -1;
	}

	errno = 0;
	int64_t tmp = json_object_get_int64(value);
	if (errno) {
		log_err("JSON: failed to parse integer from key: %s\n", key);
		return -1;
	}

	*_value = tmp;
	return 0;
}

static int json_set_internal(struct json_object *obj, const char *key, struct json_object *value,
                             unsigned flags)
{
	int ret = 0;

	ret = json_object_object_add_ex(obj, key, value, flags);
	if (ret < 0) {
		json_errno("json_object_object_add_ex");
		return ret;
	}

	return 0;
}

static int json_set_string_internal(struct json_object *obj, const char *key, const char *_value,
                                    unsigned flags)
{
	struct json_object *value = json_object_new_string(_value);
	if (!value) {
		json_errno("json_object_new_string");
		return -1;
	}

	int ret = json_set_internal(obj, key, value, flags);
	if (ret < 0)
		goto free_value;

	return ret;

free_value:
	json_object_put(value);

	return ret;
}

static int json_set_int_internal(struct json_object *obj, const char *key, int64_t _value,
                                 unsigned flags)
{
	struct json_object *value = json_object_new_int64(_value);
	if (!value) {
		json_errno("json_object_new_int");
		return -1;
	}

	int ret = json_set_internal(obj, key, value, flags);
	if (ret < 0)
		goto free_value;

	return ret;

free_value:
	json_object_put(value);

	return ret;
}

int json_set(struct json_object *obj, const char *key, struct json_object *value)
{
	return json_set_internal(obj, key, value, 0);
}

int json_set_string(struct json_object *obj, const char *key, const char *value)
{
	return json_set_string_internal(obj, key, value, 0);
}

int json_set_int(struct json_object *obj, const char *key, int64_t value)
{
	return json_set_int_internal(obj, key, value, 0);
}

#ifndef JSON_C_OBJECT_ADD_CONSTANT_KEY
#define JSON_C_OBJECT_ADD_CONSTANT_KEY JSON_C_OBJECT_KEY_IS_CONSTANT
#endif
static const unsigned json_const_key_flags = JSON_C_OBJECT_ADD_CONSTANT_KEY;

int json_set_const_key(struct json_object *obj, const char *key, struct json_object *value)
{
	return json_set_internal(obj, key, value, json_const_key_flags);
}

int json_set_string_const_key(struct json_object *obj, const char *key, const char *value)
{
	return json_set_string_internal(obj, key, value, json_const_key_flags);
}

int json_set_int_const_key(struct json_object *obj, const char *key, int64_t value)
{
	return json_set_int_internal(obj, key, value, json_const_key_flags);
}

int json_append(struct json_object *arr, struct json_object *elem)
{
	int ret = json_object_array_add(arr, elem);
	if (ret < 0) {
		json_errno("json_object_array_add");
		return ret;
	}
	return ret;
}
