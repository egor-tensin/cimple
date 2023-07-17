# Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
# This file is part of the "cimple" project.
# For details, see https://github.com/egor-tensin/cimple.
# Distributed under the MIT License.

from collections import namedtuple
import logging
import os

from pytest import fixture

from lib import test_repo as repo
from lib.db import Database
from lib.net import random_unused_port
from lib.process import CmdLine


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
    ParamBinary(name) for name in ('server', 'worker', 'client', 'sigsegv')
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
def base_cmd_line(pytestconfig):
    cmd_line = CmdLine.unbuffered()
    valgrind = pytestconfig.getoption(PARAM_VALGRIND.codename)
    if valgrind is not None:
        # Signal to Valgrind that ci scripts should obviously be exempt from
        # memory leak checking:
        cmd_line = CmdLine.wrap(CmdLine(valgrind, '--trace-children-skip=*/ci', '--'), cmd_line)
    return cmd_line


@fixture(scope='session')
def version(pytestconfig):
    return pytestconfig.getoption(PARAM_VERSION.codename)


@fixture(scope='session')
def server_port():
    return str(random_unused_port())


@fixture
def sqlite_path(tmp_path):
    return os.path.join(tmp_path, 'cimple.sqlite')


@fixture
def sqlite_db(server, sqlite_path):
    return Database(sqlite_path)


class CmdLineServer(CmdLine):
    def log_line_means_process_ready(self, line):
        return line.endswith('Waiting for new connections')


class CmdLineWorker(CmdLine):
    def log_line_means_process_ready(self, line):
        return line.endswith('Waiting for a new command')


@fixture
def server_exe(paths):
    return CmdLineServer(paths.server_binary)


@fixture
def worker_exe(paths):
    return CmdLineWorker(paths.worker_binary)


@fixture
def client_exe(paths):
    return CmdLine(paths.client_binary)


@fixture
def server_cmd(base_cmd_line, paths, server_port, sqlite_path):
    args = ['--port', server_port, '--sqlite', sqlite_path]
    return CmdLineServer.wrap(base_cmd_line, CmdLine(paths.server_binary, *args))


@fixture
def worker_cmd(base_cmd_line, paths, server_port):
    args = ['--host', '127.0.0.1', '--port', server_port]
    return CmdLineWorker.wrap(base_cmd_line, CmdLine(paths.worker_binary, *args))


@fixture
def client(base_cmd_line, paths, server_port):
    args = ['--host', '127.0.0.1', '--port', server_port]
    return CmdLine.wrap(base_cmd_line, CmdLine(paths.client_binary, *args))


@fixture
def sigsegv(paths):
    return CmdLine(paths.sigsegv_binary)


@fixture
def server(server_cmd):
    with server_cmd.run_async() as server:
        yield server
    assert server.returncode == 0


@fixture
def workers(worker_cmd):
    with worker_cmd.run_async() as worker1, \
         worker_cmd.run_async() as worker2:
        yield [worker1, worker2]
    assert worker1.returncode == 0
    assert worker2.returncode == 0


@fixture
def repo_path(tmp_path):
    return os.path.join(tmp_path, 'repo')


ALL_REPOS = [
    repo.TestRepoOutputSimple,
    repo.TestRepoOutputEmpty,
    repo.TestRepoOutputLong,
    repo.TestRepoOutputNull,
    repo.TestRepoSegfault,
]


def _make_repo(repo_path, paths, cls):
    args = [repo_path]
    if cls is repo.TestRepoSegfault:
        args += [paths.sigsegv_binary]
    return cls(*args)


@fixture(params=ALL_REPOS, ids=[repo.codename() for repo in ALL_REPOS])
def test_repo(repo_path, paths, request):
    return _make_repo(repo_path, paths, request.param)


@fixture(params=[repo for repo in ALL_REPOS if repo.enabled_for_stress_testing()],
         ids=[repo.codename() for repo in ALL_REPOS if repo.enabled_for_stress_testing()])
def stress_test_repo(repo_path, paths, request):
    return _make_repo(repo_path, paths, request.param)


Env = namedtuple('Env', ['server', 'workers', 'client', 'db'])


@fixture
def env(server, workers, client, sqlite_db):
    return Env(server, workers, client, sqlite_db)
