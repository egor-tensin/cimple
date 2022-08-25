#ifndef __FILE_H__
#define __FILE_H__

int rm_rf(const char *dir);

int my_chdir(const char *dir, char **old);

int file_exists(const char *path);

#endif
