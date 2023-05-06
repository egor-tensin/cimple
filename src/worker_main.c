/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "cmd_line.h"
#include "const.h"
#include "worker.h"

#include <getopt.h>

static struct settings default_settings()
{
	struct settings settings = {DEFAULT_HOST, DEFAULT_PORT};
	return settings;
}

const char *get_usage_string()
{
	return "[-h|--help] [-V|--version] [-H|--host HOST] [-p|--port PORT]";
}

static int parse_settings(struct settings *settings, int argc, char *argv[])
{
	int opt, longind;

	*settings = default_settings();

	static struct option long_options[] = {
	    {"help", no_argument, 0, 'h'},
	    {"version", no_argument, 0, 'V'},
	    {"host", required_argument, 0, 'H'},
	    {"port", required_argument, 0, 'p'},
	    {0, 0, 0, 0},
	};

	while ((opt = getopt_long(argc, argv, "hVH:p:", long_options, &longind)) != -1) {
		switch (opt) {
		case 'h':
			exit_with_usage(0);
			break;
		case 'V':
			exit_with_version();
			break;
		case 'H':
			settings->host = optarg;
			break;
		case 'p':
			settings->port = optarg;
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
	struct worker *worker;
	int ret = 0;

	ret = parse_settings(&settings, argc, argv);
	if (ret < 0)
		return ret;

	ret = worker_create(&worker, &settings);
	if (ret < 0)
		return ret;

	ret = worker_main(worker, argc - optind, argv + optind);
	if (ret < 0)
		goto destroy_worker;

destroy_worker:
	worker_destroy(worker);

	return ret;
}
