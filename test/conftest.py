# Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
# This file is part of the "cimple" project.
# For details, see https://github.com/egor-tensin/cimple.
# Distributed under the MIT License.

import logging
import os
import random

from pytest import fixture

from .lib.process import CmdLine, CmdLineBuilder, Runner


class Param:
    def __init__(self, codename, help_string, required=True):
        self.codename = codename
        self.help_string = help_string
        self.required = required

    @property
    def cmd_line(self):
        return f"--{self.codename.replace('_', '-')}"

    def add_to_parser(self, parser):
        parser.addoption(self.cmd_line, required=self.required, help=self.help_string)


class ParamBinary(Param):
    def __init__(self, name, **kwargs):
        self.name = name
        super().__init__(self.get_code_name(), self.get_help_string(), **kwargs)

    def get_code_name(self):
        return f'{self.name}_binary'

    @property
    def basename(self):
        return f'cimple-{self.name}'

    def get_help_string(self):
        return f'{self.basename} binary path'


BINARY_PARAMS = [
    ParamBinary(name) for name in ('server', 'worker', 'client')
]

PARAM_VALGRIND = ParamBinary('valgrind', required=False)


class ParamVersion(Param):
    def __init__(self):
        super().__init__('project_version', 'project version')


PARAM_VERSION = ParamVersion()

PARAMS = list(BINARY_PARAMS)
PARAMS += [
    PARAM_VALGRIND,
    PARAM_VERSION,
]


def pytest_addoption(parser):
    for opt in PARAMS:
        opt.add_to_parser(parser)


def pytest_generate_tests(metafunc):
    for opt in PARAMS:
        if opt.codename in metafunc.fixturenames:
            metafunc.parametrize(opt.codename, metafunc.config.getoption(opt.codename))


@fixture(scope='session')
def rng():
    random.seed()


class Paths:
    def __init__(self, pytestconfig):
        for opt in BINARY_PARAMS:
            setattr(self, opt.codename, None)
        for opt in BINARY_PARAMS:
            path = pytestconfig.getoption(opt.codename)
            if path is None:
                continue
            logging.info('%s path: %s', opt.basename, path)
            setattr(self, opt.codename, path)


@fixture(scope='session')
def paths(pytestconfig):
    return Paths(pytestconfig)


@fixture(scope='session')
def process_runner(pytestconfig):
    runner = Runner()
    valgrind = pytestconfig.getoption(PARAM_VALGRIND.codename)
    if valgrind is not None:
        runner.add_wrapper(CmdLine(valgrind))
    return runner


@fixture(scope='session')
def version(pytestconfig):
    return pytestconfig.getoption(PARAM_VERSION.codename)


@fixture
def server_port(rng):
    return str(random.randint(2000, 50000))


@fixture
def sqlite_path(tmp_path):
    return os.path.join(tmp_path, 'cimple.sqlite')


class CmdLineServer(CmdLine):
    def log_line_means_ready(self, line):
        return line.endswith('Waiting for new connections')


class CmdLineWorker(CmdLine):
    def log_line_means_ready(self, line):
        return line.endswith('Waiting for a new command')


@fixture
def server(process_runner, paths, server_port, sqlite_path):
    args = ['--port', server_port, '--sqlite', sqlite_path]
    cmd_line = CmdLineServer(paths.server_binary, *args)
    with process_runner.run_async(cmd_line) as server:
        yield
    assert server.returncode == 0


@fixture
def workers(process_runner, paths, server_port):
    args = ['--host', '127.0.0.1', '--port', server_port]
    cmd_line = CmdLineWorker(paths.worker_binary, *args)
    with process_runner.run_async(cmd_line) as worker1, \
         process_runner.run_async(cmd_line) as worker2:
        yield
    assert worker1.returncode == 0
    assert worker2.returncode == 0


@fixture
def server_and_workers(server, workers):
    yield


@fixture
def client(process_runner, paths, server_port):
    args = ['--port', server_port]
    cmd_line = CmdLineBuilder(process_runner, paths.client_binary, *args)
    return cmd_line
