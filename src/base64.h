/*
 * Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __BASE64_H__
#define __BASE64_H__

#include <stddef.h>

int base64_encode(const unsigned char *src, size_t src_len, char **dst);
int base64_decode(const char *src, unsigned char **dst, size_t *dst_len);

#endif
