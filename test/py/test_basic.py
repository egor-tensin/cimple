# Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
# This file is part of the "cimple" project.
# For details, see https://github.com/egor-tensin/cimple.
# Distributed under the MIT License.

import re


def _test_cmd_line_version_internal(cmd_line, name, version):
    for flag in ('--version', '-V'):
        output = cmd_line.run(flag).removesuffix('\n')
        match = re.match(r'^cimple-(\w+) v(\d+\.\d+\.\d+) \([0-9a-f]{40,}\)$', output)
        assert match, f'Invalid {flag} output:\n{output}'
        assert match.group(1) == name
        assert match.group(2) == version


def test_cmd_line_version(server_exe, worker_exe, client_exe, version):
    _test_cmd_line_version_internal(server_exe, 'server', version)
    _test_cmd_line_version_internal(worker_exe, 'worker', version)
    _test_cmd_line_version_internal(client_exe, 'client', version)


def _test_cmd_line_help_internal(cmd_line, name):
    for flag in ('--help', '-h'):
        output = cmd_line.run(flag).removesuffix('\n')
        match = re.match(r'^usage: cimple-(\w+) ', output)
        assert match, f'Invalid {flag} output:\n{output}'
        assert match.group(1) == name


def test_cmd_line_help(server_exe, worker_exe, client_exe):
    _test_cmd_line_help_internal(server_exe, 'server')
    _test_cmd_line_help_internal(worker_exe, 'worker')
    _test_cmd_line_help_internal(client_exe, 'client')


def _test_cmd_line_invalid_option_internal(cmd_line, name):
    for args in (['-x'], ['--invalid', 'value']):
        ec, output = cmd_line.try_run(*args)
        assert ec != 0, f'Invalid exit code {ec}, output:\n{output}'


def test_cmd_line_invalid_option(server_exe, worker_exe, client_exe):
    _test_cmd_line_invalid_option_internal(server_exe, 'server')
    _test_cmd_line_invalid_option_internal(worker_exe, 'worker')
    _test_cmd_line_invalid_option_internal(client_exe, 'client')


def test_run_client_no_msg(client):
    ec, output = client.try_run()
    assert ec != 0, f'Invalid exit code {ec}, output:\n{output}'
    prefix = 'usage error: no message to send to the server\n'
    assert output.startswith(prefix), f'Invalid output:\n{output}'


def test_run_client_invalid_msg(server, client):
    ec, output = client.try_run('hello')
    assert ec != 0, f'Invalid exit code {ec}, output:\n{output}'
    suffix = "Failed to connect to server or it couldn't process the request\n"
    assert output.endswith(suffix), f'Invalid output:\n{output}'


def test_run_noop_server(server):
    pass


def test_run_noop_server_and_workers(server_and_workers):
    pass
