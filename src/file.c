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
		char *tmp_buf = realloc(buf, current_size);
		if (!tmp_buf) {
			log_errno("realloc");
			goto free;
		}
		buf = tmp_buf;

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

void file_close(int fd)
{
	log_errno_if(close(fd), "close");
}

int file_exists(const char *path)
{
	struct stat stat;
	int ret = lstat(path, &stat);
	return !ret && S_ISREG(stat.st_mode);
}

int file_read(int fd, char **_contents, size_t *_size)
{
	size_t alloc_size = 256;
	char *contents = NULL;
	size_t size = 0;

	while (1) {
		char *tmp_contents = realloc(contents, alloc_size);
		if (!tmp_contents) {
			log_errno("realloc");
			free(contents);
			return -1;
		}
		contents = tmp_contents;

		ssize_t read_size = read(fd, contents + size, alloc_size - size - 1);

		if (read_size < 0) {
			log_errno("read");
			free(contents);
			return read_size;
		}

		if (!read_size) {
			*_contents = contents;
			*_size = size;
			return 0;
		}

		size += read_size;
		contents[size] = '\0';

		if (size == alloc_size - 1) {
			alloc_size *= 2;
		}
	}
}
