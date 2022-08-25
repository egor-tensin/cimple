#include "file.h"
#include "log.h"

#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

static int unlink_cb(const char *fpath, const struct stat *, int, struct FTW *)
{
	int ret = 0;

	ret = remove(fpath);
	if (ret < 0) {
		print_errno("remove");
		return ret;
	}

	return ret;
}

int rm_rf(const char *dir)
{
	print_log("Recursively removing directory: %s\n", dir);
	return nftw(dir, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

int my_chdir(const char *dir, char **old)
{
	int ret = 0;

	if (old) {
		*old = get_current_dir_name();
		if (!*old) {
			print_errno("get_current_dir_name");
			return -1;
		}
	}

	ret = chdir(dir);
	if (ret < 0) {
		print_errno("chdir");
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
