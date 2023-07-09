/*
 * Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "base64.h"
#include "log.h"

#include <sodium.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static const int base64_variant = sodium_base64_VARIANT_ORIGINAL;

int base64_encode(const unsigned char *src, size_t src_len, char **_dst)
{
	const size_t dst_len = sodium_base64_encoded_len(src_len, base64_variant);

	char *dst = calloc(dst_len, 1);
	if (!dst) {
		log_errno("calloc");
		return -1;
	}

	sodium_bin2base64(dst, dst_len, src, src_len, base64_variant);

	*_dst = dst;
	return 0;
}

int base64_decode(const char *src, unsigned char **_dst, size_t *_dst_len)
{
	const size_t src_len = strlen(src);
	const size_t dst_max_len = src_len / 4 * 3;
	size_t dst_len = 0;

	unsigned char *dst = calloc(dst_max_len, 1);
	if (!dst) {
		log_errno("calloc");
		return -1;
	}

	int ret =
	    sodium_base642bin(dst, dst_max_len, src, src_len, NULL, &dst_len, NULL, base64_variant);
	if (ret < 0) {
		log_err("Couldn't parse base64-encoded string\n");
		goto free;
	}

	*_dst = dst;
	*_dst_len = dst_len;
	return ret;

free:
	free(dst);

	return ret;
}
