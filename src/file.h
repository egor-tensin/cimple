#ifndef __FILE_H__
#define __FILE_H__

#include <stddef.h>

int rm_rf(const char *dir);

int my_chdir(const char *dir, char **old);

int file_exists(const char *path);

int file_read(int fd, char **output, size_t *len);

#endif
