/*
 * Copyright (c) 2023 Egor Tensin <egor@tensin.name>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "buf.h"
#include "log.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct buf {
	uint32_t size;
	const void *data;
};

int buf_create(struct buf **_buf, const void *data, uint32_t size)
{
	struct buf *buf = malloc(sizeof(struct buf));
	if (!buf) {
		log_errno("malloc");
		return -1;
	}

	buf->data = data;
	buf->size = size;

	*_buf = buf;
	return 0;
}

int buf_create_from_string(struct buf **buf, const char *str)
{
	return buf_create(buf, str, strlen(str) + 1);
}

void buf_destroy(struct buf *buf)
{
	free(buf);
}

uint32_t buf_get_size(const struct buf *buf)
{
	return buf->size;
}

const void *buf_get_data(const struct buf *buf)
{
	return buf->data;
}
