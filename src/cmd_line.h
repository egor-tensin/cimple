/*
 * Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#ifndef __CMD_LINE_H__
#define __CMD_LINE_H__

const char *get_usage_string();

void exit_with_usage(int ec, const char *argv0);
void exit_with_usage_err(const char *argv0, const char *msg);
void exit_with_version();

#endif
