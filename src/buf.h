/*
 * Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __BUF_H__
#define __BUF_H__

#include <stdint.h>

struct buf;

int buf_create(struct buf **, const void *, uint32_t);
int buf_create_from_string(struct buf **, const char *);
void buf_destroy(struct buf *);

uint32_t buf_get_size(const struct buf *);
const void *buf_get_data(const struct buf *);

#endif
