/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "cmd_line.h"
#include "const.h"
#include "log.h"
#include "server.h"

#include <getopt.h>
#include <unistd.h>

static struct settings default_settings(void)
{
	struct settings settings = {default_port, default_sqlite_path};
	return settings;
}

const char *get_usage_string(void)
{
	return "[-h|--help] [-V|--version] [-v|--verbose] [-p|--port PORT] [-s|--sqlite PATH]";
}

static int parse_settings(struct settings *settings, int argc, char *argv[])
{
	int opt, longind;

	*settings = default_settings();

	static struct option long_options[] = {
	    {"help", no_argument, 0, 'h'},
	    {"version", no_argument, 0, 'V'},
	    {"verbose", no_argument, 0, 'v'},
	    {"port", required_argument, 0, 'p'},
	    {"sqlite", required_argument, 0, 's'},
	    {0, 0, 0, 0},
	};

	while ((opt = getopt_long(argc, argv, "hVvp:s:", long_options, &longind)) != -1) {
		switch (opt) {
		case 'h':
			exit_with_usage(0);
			break;
		case 'V':
			exit_with_version();
			break;
		case 'v':
			g_log_lvl = LOG_LVL_DEBUG;
			break;
		case 'p':
			settings->port = optarg;
			break;
		case 's':
			settings->sqlite_path = optarg;
			break;
		default:
			exit_with_usage(1);
			break;
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	struct settings settings;
	struct server *server = NULL;
	int ret = 0;

	ret = parse_settings(&settings, argc, argv);
	if (ret < 0)
		return ret;

	ret = server_create(&server, &settings);
	if (ret < 0)
		return ret;

	ret = server_main(server);
	if (ret < 0)
		goto destroy_server;

destroy_server:
	server_destroy(server);

	return ret;
}
