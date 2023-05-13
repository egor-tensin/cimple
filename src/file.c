/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "file.h"
#include "compiler.h"
#include "log.h"

#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int unlink_cb(const char *fpath, UNUSED const struct stat *sb, UNUSED int typeflag,
                     UNUSED struct FTW *ftwbuf)
{
	int ret = 0;

	ret = remove(fpath);
	if (ret < 0) {
		log_errno("remove");
		return ret;
	}

	return ret;
}

int rm_rf(const char *dir)
{
	log("Recursively removing directory: %s\n", dir);
	return nftw(dir, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

int my_chdir(const char *dir, char **old)
{
	int ret = 0;

	if (old) {
		*old = get_current_dir_name();
		if (!*old) {
			log_errno("get_current_dir_name");
			return -1;
		}
	}

	ret = chdir(dir);
	if (ret < 0) {
		log_errno("chdir");
		goto free_old;
	}

	return ret;

free_old:
	if (old)
		free(*old);

	return ret;
}

char *my_readlink(const char *path)
{
	size_t current_size = 256;
	char *buf = NULL;

	while (1) {
		buf = realloc(buf, current_size);
		if (!buf) {
			log_errno("realloc");
			goto free;
		}

		ssize_t res = readlink(path, buf, current_size);
		if (res < 0) {
			log_errno("readlink");
			goto free;
		}

		if ((size_t)res == current_size) {
			current_size *= 2;
			continue;
		}

		break;
	}

	return buf;

free:
	free(buf);

	return NULL;
}

int file_exists(const char *path)
{
	struct stat stat;
	int ret = lstat(path, &stat);
	return !ret && S_ISREG(stat.st_mode);
}

int file_read(int fd, char **_contents, size_t *_len)
{
	char buf[128];
	size_t buf_len = sizeof(buf) / sizeof(buf[0]);
	int ret = 0;

	char *contents = NULL;
	size_t len = 0;

	while (1) {
		ssize_t read_now = read(fd, buf, buf_len);

		if (read_now < 0) {
			log_errno("read");
			ret = read_now;
			goto free_output;
		}

		if (!read_now) {
			*_contents = contents;
			*_len = len;
			goto exit;
		}

		contents = realloc(contents, len + read_now + 1);
		if (!contents) {
			log_errno("realloc");
			return -1;
		}
		memcpy(contents + len, buf, read_now);
		len += read_now;
		contents[len] = '\0';
	}

free_output:
	free(contents);

exit:
	return ret;
}
