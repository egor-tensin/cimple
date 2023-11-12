/*
 * Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __JSON_RPC_H__
#define __JSON_RPC_H__

/* This attempts to adhere to the format described in https://www.jsonrpc.org/specification. */

#include <json-c/json_object.h>

#include <stdint.h>

struct jsonrpc_request;

int jsonrpc_generate_request_id(void);

int jsonrpc_request_create(struct jsonrpc_request **, int id, const char *method,
                           struct json_object *params);
void jsonrpc_request_destroy(struct jsonrpc_request *);

int jsonrpc_notification_create(struct jsonrpc_request **, const char *method,
                                struct json_object *params);
int jsonrpc_request_is_notification(const struct jsonrpc_request *);

int jsonrpc_request_send(const struct jsonrpc_request *, int fd);
int jsonrpc_request_recv(struct jsonrpc_request **, int fd);

const char *jsonrpc_request_get_method(const struct jsonrpc_request *);

int jsonrpc_request_get_param_string(const struct jsonrpc_request *, const char *name,
                                     const char **);
int jsonrpc_request_set_param_string(struct jsonrpc_request *, const char *name, const char *);
int jsonrpc_request_get_param_int(const struct jsonrpc_request *, const char *name, int64_t *);
int jsonrpc_request_set_param_int(struct jsonrpc_request *, const char *name, int64_t);

struct jsonrpc_response;

int jsonrpc_response_create(struct jsonrpc_response **, const struct jsonrpc_request *,
                            struct json_object *result);
void jsonrpc_response_destroy(struct jsonrpc_response *);

int jsonrpc_error_create(struct jsonrpc_response **, struct jsonrpc_request *, int code,
                         const char *message);
int jsonrpc_response_is_error(const struct jsonrpc_response *);

int jsonrpc_response_send(const struct jsonrpc_response *, int fd);
int jsonrpc_response_recv(struct jsonrpc_response **, int fd);

#endif
