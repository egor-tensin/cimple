/*
 * Copyright (c) 2022 Egor Tensin <Egor.Tensin@gmail.com>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "client.h"
#include "const.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

static struct settings default_settings()
{
	struct settings settings = {DEFAULT_HOST, DEFAULT_PORT};
	return settings;
}

static void print_usage(const char *argv0)
{
	printf("usage: %s [-h|--help] [-V|--version] [-H|--host HOST] [-p|--port PORT]\n", argv0);
}

static void print_version()
{
	printf("%s\n", VERSION);
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
			print_usage(argv[0]);
			exit(0);
			break;
		case 'V':
			print_version();
			exit(0);
			break;
		case 'H':
			settings->host = optarg;
			break;
		case 'p':
			settings->port = optarg;
			break;
		default:
			print_usage(argv[0]);
			exit(1);
			break;
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	struct settings settings;
	struct client client;
	int ret = 0;

	ret = parse_settings(&settings, argc, argv);
	if (ret < 0)
		return ret;

	ret = client_create(&client, &settings);
	if (ret < 0)
		return ret;

	ret = client_main(&client, argc - optind, argv + optind);
	if (ret < 0)
		goto destroy_client;

destroy_client:
	client_destroy(&client);

	return ret;
}
