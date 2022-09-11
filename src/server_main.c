#include "const.h"
#include "server.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

static struct settings default_settings()
{
	struct settings settings = {DEFAULT_PORT, NULL};
	return settings;
}

static void exit_with_usage(int ec, const char *argv0)
{
	FILE *dest = stdout;
	if (ec)
		dest = stderr;

	fprintf(dest, "usage: %s [-h|--help] [-V|--version] [-p|--port PORT] [-s|--sqlite PATH]\n",
	        argv0);
	exit(ec);
}

static void exit_with_usage_err(const char *argv0, const char *msg)
{
	if (msg)
		fprintf(stderr, "usage error: %s\n", msg);
	exit_with_usage(1, argv0);
}

static void exit_with_version()
{
	printf("%s\n", VERSION);
	exit(0);
}

static int parse_settings(struct settings *settings, int argc, char *argv[])
{
	int opt, longind;

	*settings = default_settings();

	static struct option long_options[] = {
	    {"help", no_argument, 0, 'h'},
	    {"version", no_argument, 0, 'V'},
	    {"port", required_argument, 0, 'p'},
	    {"sqlite", required_argument, 0, 's'},
	    {0, 0, 0, 0},
	};

	while ((opt = getopt_long(argc, argv, "hVp:s:", long_options, &longind)) != -1) {
		switch (opt) {
		case 'h':
			exit_with_usage(0, argv[0]);
			break;
		case 'V':
			exit_with_version();
			break;
		case 'p':
			settings->port = optarg;
			break;
		case 's':
			settings->sqlite_path = optarg;
			break;
		default:
			exit_with_usage(1, argv[0]);
			break;
		}
	}

	if (!settings->sqlite_path) {
		exit_with_usage_err(argv[0], "must specify the path to a SQLite database");
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	struct settings settings;
	struct server server;
	int ret = 0;

	ret = parse_settings(&settings, argc, argv);
	if (ret < 0)
		return ret;

	ret = server_create(&server, &settings);
	if (ret < 0)
		return ret;

	ret = server_main(&server);
	if (ret < 0)
		goto destroy_server;

destroy_server:
	server_destroy(&server);

	return ret;
}
