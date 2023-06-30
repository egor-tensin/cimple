# Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
# This file is part of the "cimple" project.
# For details, see https://github.com/egor-tensin/cimple.
# Distributed under the MIT License.

import re


def test_server_and_workers_run(server_and_workers):
    pass


def test_client_version(client, version):
    output = client.run('--version').removesuffix('\n')
    match = re.match(r'^cimple-client v(\d+\.\d+\.\d+) \([0-9a-f]{40,}\)$', output)
    assert match, f'Invalid --version output: {output}'
    assert match.group(1) == version
