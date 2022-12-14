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

int file_exists(const char *path)
{
	struct stat stat;
	int ret = lstat(path, &stat);
	return !ret && S_ISREG(stat.st_mode);
}

int file_read(int fd, char **output, size_t *len)
{
	char buf[128];
	size_t buf_len = sizeof(buf) / sizeof(buf[0]);
	int ret = 0;

	*output = NULL;
	*len = 0;

	while (1) {
		ssize_t read_now = read(fd, buf, buf_len);

		if (read_now < 0) {
			log_errno("read");
			ret = read_now;
			goto free_output;
		}

		if (!read_now)
			goto exit;

		*output = realloc(*output, *len + read_now + 1);
		if (!*output) {
			log_errno("realloc");
			return -1;
		}
		memcpy(*output + *len, buf, read_now);
		*len += read_now;
		*(*output + *len) = '\0';
	}

free_output:
	free(*output);

exit:
	return ret;
}
