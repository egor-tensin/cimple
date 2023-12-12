/*
 * Copyright (c) 2022 Egor Tensin <egor@tensin.name>
 * This file is part of the "cimple" project.
 * For details, see https://github.com/egor-tensin/cimple.
 * Distributed under the MIT License.
 */

#include "client.h"
#include "cmd_line.h"
#include "const.h"
#include "log.h"

#include <getopt.h>
#include <unistd.h>

static struct settings default_settings(void)
{
	struct settings settings = {
	    .host = default_host,
	    .port = default_port,
	};
	return settings;
}

const char *get_usage_string(void)
{
	return "[-h|--help] [-V|--version] [-v|--verbose] [-H|--host HOST] [-p|--port PORT] ACTION [ARG...]\n\
\n\
available actions:\n\
\t" CMD_QUEUE_RUN " URL REV - schedule a CI run of repository at URL, revision REV";
}

static int parse_settings(struct settings *settings, int argc, char *argv[])
{
	int opt, longind;

	*settings = default_settings();

	/* clang-format off */
	static struct option long_options[] = {
	    {"help", no_argument, 0, 'h'},
	    {"version", no_argument, 0, 'V'},
	    {"verbose", no_argument, 0, 'v'},
	    {"host", required_argument, 0, 'H'},
	    {"port", required_argument, 0, 'p'},
	    {0, 0, 0, 0},
	};
	/* clang-format on */

	while ((opt = getopt_long(argc, argv, "hVvH:p:", long_options, &longind)) != -1) {
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
	struct client *client = NULL;
	int ret = 0;

	ret = parse_settings(&settings, argc, argv);
	if (ret < 0)
		return ret;

	ret = client_create(&client);
	if (ret < 0)
		return ret;

	ret = client_main(client, &settings, argc - optind, (const char **)argv + optind);
	if (ret < 0)
		goto destroy_client;

destroy_client:
	client_destroy(client);

	return ret;
}
