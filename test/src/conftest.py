# Copyright (c) 2023 Egor Tensin <egor@tensin.name>
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


PARAMS = [
    Param(f'{name}', f'cimple-{name} binary path')
    for name in ('server', 'worker', 'client')
]
PARAMS += [
    Param('sigsegv', 'sigsegv binary path'),
    Param('project_version', 'project version'),
    Param('valgrind', 'path to valgrind.sh', required=False),
    Param('flamegraph', 'path to flamegraph.sh', required=False),
    Param('flame_graphs_dir', 'directory to store flame graphs', required=False),
]


def pytest_addoption(parser):
    for opt in PARAMS:
        opt.add_to_parser(parser)


class Params:
    def __init__(self, pytestconfig):
        for opt in PARAMS:
            setattr(self, opt.codename, None)
        for opt in PARAMS:
            path = pytestconfig.getoption(opt.codename)
            if path is None:
                continue
            logging.info("'%s' parameter value: %s", opt.codename, path)
            setattr(self, opt.codename, path)


@fixture(scope='session')
def params(pytestconfig):
    return Params(pytestconfig)


class CmdLineValgrind(CmdLine):
    def __init__(self, binary):
        # Signal to Valgrind that ci scripts should obviously be exempt from
        # memory leak checking:
        super().__init__(binary, '--trace-children-skip=*/ci', '--')


@fixture(scope='session')
def base_cmd_line(params):
    cmd_line = CmdLine.unbuffered()
    valgrind = params.valgrind
    if valgrind is not None:
        cmd_line = CmdLine.wrap(CmdLineValgrind(valgrind), cmd_line)
    return cmd_line


@fixture(scope='session')
def version(params):
    return params.project_version


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
def server_exe(params):
    return CmdLineServer(params.server)


@fixture
def worker_exe(params):
    return CmdLineWorker(params.worker)


@fixture
def client_exe(params):
    return CmdLine(params.client)


@fixture
def server_cmd(base_cmd_line, params, server_port, sqlite_path):
    args = ['--port', server_port, '--sqlite', sqlite_path]
    return CmdLineServer.wrap(base_cmd_line, CmdLine(params.server, *args))


@fixture
def worker_cmd(base_cmd_line, params, server_port):
    args = ['--host', '127.0.0.1', '--port', server_port]
    return CmdLineWorker.wrap(base_cmd_line, CmdLine(params.worker, *args))


@fixture
def client(base_cmd_line, params, server_port):
    args = ['--host', '127.0.0.1', '--port', server_port]
    return CmdLine.wrap(base_cmd_line, CmdLine(params.client, *args))


@fixture
def sigsegv(params):
    return CmdLine(params.sigsegv)


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
def flame_graph_svg(params, tmp_path, flame_graph_repo):
    dir = params.flame_graphs_dir
    if dir is None:
        return os.path.join(tmp_path, 'flame_graph.svg')
    os.makedirs(dir, exist_ok=True)
    return os.path.join(dir, f'flame_graph_{flame_graph_repo.codename()}.svg')


@fixture
def profiler(params, server, workers, flame_graph_svg):
    pids = [server.pid] + [worker.pid for worker in workers]
    pids = map(str, pids)
    cmd_line = CmdLine(params.flamegraph, flame_graph_svg, *pids)
    with cmd_line.run_async() as proc:
        yield
    assert proc.returncode == 0


@fixture
def repo_path(tmp_path):
    return os.path.join(tmp_path, 'repo')


TEST_REPOS = [
    repo.TestRepoOutputSimple,
    repo.TestRepoOutputEmpty,
    repo.TestRepoOutputLong,
    repo.TestRepoOutputNull,
    repo.TestRepoSegfault,
]

STRESS_TEST_REPOS = [
    repo.TestRepoOutputSimple,
    repo.TestRepoOutputLong,
]


def _make_repo(repo_path, params, cls):
    args = [repo_path]
    if cls is repo.TestRepoSegfault:
        args += [params.sigsegv]
    return cls(*args)


@fixture(params=TEST_REPOS, ids=[repo.codename() for repo in TEST_REPOS])
def test_repo(repo_path, params, request):
    return _make_repo(repo_path, params, request.param)


@fixture(params=STRESS_TEST_REPOS, ids=[repo.codename() for repo in STRESS_TEST_REPOS])
def stress_test_repo(repo_path, params, request):
    return _make_repo(repo_path, params, request.param)


@fixture
def flame_graph_repo(stress_test_repo):
    return stress_test_repo


Env = namedtuple('Env', ['server', 'workers', 'client', 'db'])


@fixture
def env(server, workers, client, sqlite_db):
    return Env(server, workers, client, sqlite_db)
