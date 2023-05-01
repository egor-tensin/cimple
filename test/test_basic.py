# Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
# This file is part of the "cimple" project.
# For details, see https://github.com/egor-tensin/cimple.
# Distributed under the MIT License.

import pytest


def test_server_and_workers_run(server_and_workers):
    pass


def test_client_version(client, version):
    ec, output = client.run('--version')
    assert ec == 0
    assert output.endswith(version + '\n')
