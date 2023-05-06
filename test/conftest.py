# Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
# This file is part of the "cimple" project.
# For details, see https://github.com/egor-tensin/cimple.
# Distributed under the MIT License.

import logging
import os
import random

from pytest import fixture

from .lib.process import run, run_async


class CmdLineOption:
    def __init__(self, codename, help_string):
        self.codename = codename
        self.help_string = help_string

    @property
    def cmd_line(self):
        return f"--{self.codename.replace('_', '-')}"


class CmdLineBinary(CmdLineOption):
    def __init__(self, name):
        self.name = name
        super().__init__(self.get_code_name(), self.get_help_string())

    def get_code_name(self):
        return f'{self.name}_binary'

    @property
    def basename(self):
        return f'cimple-{self.name}'

    def get_help_string(self):
        return f'{self.basename} binary path'


CMD_LINE_BINARIES = [CmdLineBinary(name) for name in ('server', 'worker', 'client')]


class CmdLineVersion(CmdLineOption):
    def __init__(self):
        super().__init__('project_version', 'project version')


CMD_LINE_VERSION = CmdLineVersion()
CMD_LINE_OPTIONS = CMD_LINE_BINARIES + [CMD_LINE_VERSION]


def pytest_addoption(parser):
    for opt in CMD_LINE_OPTIONS:
        parser.addoption(opt.cmd_line, required=True, help=opt.help_string)


def pytest_generate_tests(metafunc):
    for opt in CMD_LINE_OPTIONS:
        if opt.codename in metafunc.fixturenames:
            metafunc.parametrize(opt.codename, metafunc.config.getoption(opt.codename))


@fixture(scope='session')
def rng():
    random.seed()


class Paths:
    def __init__(self, pytestconfig):
        for binary in CMD_LINE_BINARIES:
            path = pytestconfig.getoption(binary.codename)
            logging.info('%s path: %s', binary.basename, path)
            setattr(self, binary.codename, path)


@fixture(scope='session')
def paths(pytestconfig):
    return Paths(pytestconfig)


@fixture(scope='session')
def version(pytestconfig):
    return pytestconfig.getoption(CMD_LINE_VERSION.codename)


@fixture
def server_port(rng):
    return str(random.randint(2000, 50000))


@fixture
def sqlite_path(tmp_path):
    return os.path.join(tmp_path, 'cimple.sqlite')


@fixture
def server(paths, server_port, sqlite_path):
    with run_async(paths.server_binary, '--port', server_port, '--sqlite', sqlite_path) as server:
        yield
    assert server.returncode == 0


@fixture
def workers(paths, server_port):
    args = [paths.worker_binary, '--host', '127.0.0.1', '--port', server_port]
    with run_async(*args) as worker1, run_async(*args) as worker2:
        yield
    assert worker1.returncode == 0
    assert worker2.returncode == 0


@fixture
def server_and_workers(server, workers):
    yield


class Client:
    def __init__(self, binary):
        self.binary = binary

    def run(self, *args):
        return run(self.binary, *args)


@fixture
def client(paths):
    return Client(paths.client_binary)
