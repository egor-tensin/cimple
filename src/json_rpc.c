/*
 * Copyright (c) 2023 Egor Tensin <egor@tensin.name>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "json_rpc.h"
#include "json.h"
#include "log.h"

#include <json-c/json_object.h>

#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct jsonrpc_request {
	struct json_object *impl;
};

struct jsonrpc_response {
	struct json_object *impl;
};

static const char *const jsonrpc_key_version = "jsonrpc";
static const char *const jsonrpc_key_id = "id";
static const char *const jsonrpc_key_method = "method";
static const char *const jsonrpc_key_params = "params";

static const char *const jsonrpc_value_version = "2.0";

static int jsonrpc_check_version(struct json_object *obj)
{
	const char *key = jsonrpc_key_version;
	const char *version = NULL;

	int ret = json_get_string(obj, key, &version);
	if (ret < 0)
		return ret;

	if (strcmp(version, jsonrpc_value_version)) {
		log_err("JSON-RPC: invalid '%s' value: %s\n", key, version);
		return -1;
	}

	return 0;
}

static int jsonrpc_set_version(struct json_object *obj)
{
	return json_set_string_const_key(obj, jsonrpc_key_version, jsonrpc_value_version);
}

static _Atomic int jsonrpc_id_counter = 1;

int jsonrpc_generate_request_id(void)
{
	return jsonrpc_id_counter++;
}

static int jsonrpc_check_id_type(struct json_object *id)
{
	if (!json_object_is_type(id, json_type_string) && !json_object_is_type(id, json_type_int)) {
		log_err("JSON-RPC: key '%s' must be either an integer or a string\n",
		        jsonrpc_key_id);
		return -1;
	}
	return 0;
}

static int jsonrpc_check_id(struct json_object *obj, int required)
{
	const char *key = jsonrpc_key_id;

	if (!json_has(obj, key)) {
		if (!required)
			return 0;
		log_err("JSON-RPC: key is missing: %s\n", key);
		return -1;
	}

	struct json_object *id = NULL;

	int ret = json_get(obj, key, &id);
	if (ret < 0)
		return ret;
	return jsonrpc_check_id_type(id);
}

static int jsonrpc_set_id(struct json_object *obj, int id)
{
	return json_set_int_const_key(obj, jsonrpc_key_id, id);
}

static int jsonrpc_check_method(struct json_object *obj)
{
	const char *key = jsonrpc_key_method;
	const char *method = NULL;
	return json_get_string(obj, key, &method);
}

static int jsonrpc_set_method(struct json_object *obj, const char *method)
{
	return json_set_string_const_key(obj, jsonrpc_key_method, method);
}

static int jsonrpc_check_params_type(struct json_object *params)
{
	if (!json_object_is_type(params, json_type_object) &&
	    !json_object_is_type(params, json_type_array)) {
		log_err("JSON-RPC: key '%s' must be either an object or an array\n",
		        jsonrpc_key_params);
		return -1;
	}
	return 0;
}

static int jsonrpc_check_params(struct json_object *obj)
{
	const char *key = jsonrpc_key_params;

	if (!json_has(obj, key))
		return 0;

	struct json_object *params = NULL;

	int ret = json_get(obj, key, &params);
	if (ret < 0)
		return ret;
	return jsonrpc_check_params_type(params);
}

static int jsonrpc_set_params(struct json_object *obj, struct json_object *params)
{
	const char *key = jsonrpc_key_params;

	int ret = jsonrpc_check_params_type(params);
	if (ret < 0)
		return ret;
	return json_set_const_key(obj, key, params);
}

static const char *const jsonrpc_key_result = "result";
static const char *const jsonrpc_key_error = "error";

static const char *const jsonrpc_key_code = "code";
static const char *const jsonrpc_key_message = "message";

static int jsonrpc_check_error(struct json_object *obj)
{
	const char *key = jsonrpc_key_error;
	struct json_object *error = NULL;

	int ret = json_get(obj, key, &error);
	if (ret < 0)
		return ret;

	int64_t code = -1;

	ret = json_get_int(error, jsonrpc_key_code, &code);
	if (ret < 0) {
		log_err("JSON-RPC: key is missing or not an integer: %s\n", jsonrpc_key_code);
		return -1;
	}

	const char *message = NULL;

	ret = json_get_string(error, jsonrpc_key_message, &message);
	if (ret < 0) {
		log_err("JSON-RPC: key is missing or not a string: %s\n", jsonrpc_key_message);
		return -1;
	}

	return ret;
}

static int jsonrpc_check_result_or_error(struct json_object *obj)
{
	const char *key_result = jsonrpc_key_result;
	const char *key_error = jsonrpc_key_error;

	if (!json_has(obj, key_result) && !json_has(obj, key_error)) {
		log_err("JSON-RPC: either '%s' or '%s' must be present\n", key_result, key_error);
		return -1;
	}

	if (json_has(obj, key_result))
		return 0;

	return jsonrpc_check_error(obj);
}

static int jsonrpc_request_create_internal(struct jsonrpc_request **_request, int *id,
                                           const char *method, struct json_object *params)
{
	int ret = 0;

	struct jsonrpc_request *request = malloc(sizeof(struct jsonrpc_request));
	if (!request) {
		log_errno("malloc");
		ret = -1;
		goto exit;
	}

	ret = json_new_object(&request->impl);
	if (ret < 0)
		goto free;

	ret = jsonrpc_set_version(request->impl);
	if (ret < 0)
		goto free_impl;

	if (id) {
		ret = jsonrpc_set_id(request->impl, *id);
		if (ret < 0)
			goto free_impl;
	}

	ret = jsonrpc_set_method(request->impl, method);
	if (ret < 0)
		goto free_impl;

	if (params) {
		ret = jsonrpc_set_params(request->impl, params);
		if (ret < 0)
			goto free_impl;
	}

	*_request = request;
	goto exit;

free_impl:
	json_free(request->impl);
free:
	free(request);
exit:
	return ret;
}

int jsonrpc_request_create(struct jsonrpc_request **_request, int id, const char *method,
                           struct json_object *params)
{
	return jsonrpc_request_create_internal(_request, &id, method, params);
}

void jsonrpc_request_destroy(struct jsonrpc_request *request)
{
	json_free(request->impl);
	free(request);
}

int jsonrpc_notification_create(struct jsonrpc_request **_request, const char *method,
                                struct json_object *params)
{
	return jsonrpc_request_create_internal(_request, NULL, method, params);
}

int jsonrpc_request_is_notification(const struct jsonrpc_request *request)
{
	return !json_has(request->impl, jsonrpc_key_id);
}

static int jsonrpc_request_from_json(struct jsonrpc_request **_request, struct json_object *impl)
{
	int ret = 0;

	ret = jsonrpc_check_version(impl);
	if (ret < 0)
		return ret;
	ret = jsonrpc_check_id(impl, 0);
	if (ret < 0)
		return ret;
	ret = jsonrpc_check_method(impl);
	if (ret < 0)
		return ret;
	ret = jsonrpc_check_params(impl);
	if (ret < 0)
		return ret;

	struct jsonrpc_request *request = malloc(sizeof(struct jsonrpc_request));
	if (!request) {
		log_errno("malloc");
		return -1;
	}
	request->impl = impl;

	*_request = request;
	return ret;
}

int jsonrpc_request_send(const struct jsonrpc_request *request, int fd)
{
	return json_send(request->impl, fd);
}

int jsonrpc_request_recv(struct jsonrpc_request **request, int fd)
{
	struct json_object *impl = json_recv(fd);
	if (!impl) {
		log_err("JSON-RPC: failed to receive request\n");
		return -1;
	}

	int ret = jsonrpc_request_from_json(request, impl);
	if (ret < 0)
		goto free_impl;

	return ret;

free_impl:
	json_free(impl);

	return ret;
}

const char *jsonrpc_request_get_method(const struct jsonrpc_request *request)
{
	const char *method = NULL;
	int ret = json_get_string(request->impl, jsonrpc_key_method, &method);
	if (ret < 0) {
		/* Should never happen. */
		return NULL;
	}
	return method;
}

static struct json_object *jsonrpc_request_create_params(struct jsonrpc_request *request)
{
	int ret = 0;
	const char *const key = jsonrpc_key_params;

	if (!json_has(request->impl, key)) {
		struct json_object *params = NULL;

		ret = json_new_object(&params);
		if (ret < 0)
			return NULL;

		ret = json_set(request->impl, key, params);
		if (ret < 0) {
			json_free(params);
			return NULL;
		}
		return params;
	}

	struct json_object *params = NULL;
	ret = json_get(request->impl, key, &params);
	if (ret < 0)
		return NULL;
	return params;
}

int jsonrpc_request_get_param_string(const struct jsonrpc_request *request, const char *name,
                                     const char **value)
{
	struct json_object *params = NULL;
	int ret = json_get(request->impl, jsonrpc_key_params, &params);
	if (ret < 0)
		return ret;
	return json_get_string(params, name, value);
}

int jsonrpc_request_set_param_string(struct jsonrpc_request *request, const char *name,
                                     const char *value)
{
	struct json_object *params = jsonrpc_request_create_params(request);
	if (!params)
		return -1;
	return json_set_string(params, name, value);
}

int jsonrpc_request_get_param_int(const struct jsonrpc_request *request, const char *name,
                                  int64_t *value)
{
	struct json_object *params = NULL;
	int ret = json_get(request->impl, jsonrpc_key_params, &params);
	if (ret < 0)
		return ret;
	return json_get_int(params, name, value);
}

int jsonrpc_request_set_param_int(struct jsonrpc_request *request, const char *name, int64_t value)
{
	struct json_object *params = jsonrpc_request_create_params(request);
	if (!params)
		return -1;
	return json_set_int(params, name, value);
}

const char *jsonrpc_response_to_string(const struct jsonrpc_response *response)
{
	return json_to_string_pretty(response->impl);
}

int jsonrpc_response_create_internal(struct jsonrpc_response **_response,
                                     const struct jsonrpc_request *request,
                                     struct json_object *result, struct json_object *error)
{
	int ret = 0;

	struct jsonrpc_response *response = malloc(sizeof(struct jsonrpc_response));
	if (!response) {
		log_errno("malloc");
		ret = -1;
		goto exit;
	}

	ret = json_new_object(&response->impl);
	if (ret < 0)
		goto free;

	ret = jsonrpc_set_version(response->impl);
	if (ret < 0)
		goto free_impl;

	struct json_object *id = NULL;
	ret = json_clone(request->impl, jsonrpc_key_id, &id);
	if (ret < 0)
		goto free_impl;

	ret = json_set(response->impl, jsonrpc_key_id, id);
	if (ret < 0) {
		json_free(id);
		goto free_impl;
	}

	if (error) {
		ret = json_set_const_key(response->impl, jsonrpc_key_error, error);
		if (ret < 0)
			goto free_impl;
	} else {
		ret = json_set_const_key(response->impl, jsonrpc_key_result, result);
		if (ret < 0)
			goto free_impl;
	}

	*_response = response;
	goto exit;

free_impl:
	json_free(response->impl);
free:
	free(response);
exit:
	return ret;
}

int jsonrpc_response_create(struct jsonrpc_response **response,
                            const struct jsonrpc_request *request, struct json_object *result)
{
	return jsonrpc_response_create_internal(response, request, result, NULL);
}

void jsonrpc_response_destroy(struct jsonrpc_response *response)
{
	json_free(response->impl);
	free(response);
}

int jsonrpc_error_create(struct jsonrpc_response **response, struct jsonrpc_request *request,
                         int code, const char *message)
{
	int ret = 0;
	struct json_object *error = NULL;

	ret = json_new_object(&error);
	if (ret < 0)
		return ret;

	ret = json_set_int_const_key(error, jsonrpc_key_code, code);
	if (ret < 0)
		goto free;
	ret = json_set_string_const_key(error, jsonrpc_key_message, message);
	if (ret < 0)
		goto free;

	ret = jsonrpc_response_create_internal(response, request, NULL, error);
	if (ret < 0)
		goto free;

	return ret;

free:
	json_free(error);

	return ret;
}

int jsonrpc_response_is_error(const struct jsonrpc_response *response)
{
	return json_has(response->impl, jsonrpc_key_error);
}

static int jsonrpc_response_from_json(struct jsonrpc_response **_response, struct json_object *impl)
{
	int ret = 0;

	ret = jsonrpc_check_version(impl);
	if (ret < 0)
		return ret;
	ret = jsonrpc_check_id(impl, 1);
	if (ret < 0)
		return ret;
	ret = jsonrpc_check_result_or_error(impl);
	if (ret < 0)
		return ret;

	struct jsonrpc_response *response = malloc(sizeof(struct jsonrpc_response));
	if (!response) {
		log_errno("malloc");
		return -1;
	}
	response->impl = impl;

	*_response = response;
	return ret;
}

int jsonrpc_response_send(const struct jsonrpc_response *response, int fd)
{
	return json_send(response->impl, fd);
}

int jsonrpc_response_recv(struct jsonrpc_response **response, int fd)
{
	struct json_object *impl = json_recv(fd);
	if (!impl) {
		log_err("JSON-RPC: failed to receive response\n");
		return -1;
	}

	int ret = jsonrpc_response_from_json(response, impl);
	if (ret < 0)
		goto free_impl;

	return ret;

free_impl:
	json_free(impl);

	return ret;
}
