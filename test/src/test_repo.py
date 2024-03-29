# Copyright (c) 2023 Egor Tensin <egor@tensin.name>
# This file is part of the "cimple" project.
# For details, see https://github.com/egor-tensin/cimple.
# Distributed under the MIT License.

import json
import logging
import multiprocessing as mp
import re

import pytest

from lib.logging import child_logging_thread, configure_logging_in_child
from lib.process import LoggingEvent
from lib.tests import my_parametrize


def test_sigsegv(sigsegv):
    ec, output = sigsegv.try_run()
    assert ec == -11
    assert output == 'Started the test program.\n'


class LoggingEventRunComplete(LoggingEvent):
    def __init__(self, target):
        self.counter = 0
        self.target = target
        self.re = re.compile(r'run \d+ as finished')
        super().__init__(timeout=150)

    def log_line_matches(self, line):
        return bool(self.re.search(line))

    def set(self):
        self.counter += 1
        if self.counter == self.target:
            super().set()


def client_runner_process(log_queue, client, runs_per_process, repo):
    with configure_logging_in_child(log_queue):
        logging.info('Executing %s clients', runs_per_process)
        for i in range(runs_per_process):
            client.run('queue-run', repo.path, 'HEAD')


def _test_repo_internal(env, repo, numof_processes, runs_per_process):
    numof_runs = numof_processes * runs_per_process

    event = LoggingEventRunComplete(numof_runs)
    # Count the number of times the server receives the "run complete" message.
    env.server.logger.add_event(event)

    with child_logging_thread() as log_queue:
        ctx = mp.get_context('spawn')
        args = (log_queue, env.client, runs_per_process, repo)
        processes = [ctx.Process(target=client_runner_process, args=args) for i in range(numof_processes)]

        for proc in processes:
            proc.start()
        for proc in processes:
            proc.join()
        event.wait()

    repo.run_files_are_present(numof_runs)

    runs = env.db.get_all_runs()
    assert numof_runs == len(runs)

    for id, status, ec, output, url, rev in runs:
        assert status == 'finished', f'Invalid status for run {id}: {status}'
        assert repo.run_exit_code_matches(ec), f"Exit code doesn't match: {ec}"
        assert repo.run_output_matches(output), f"Output doesn't match: {output}"

    runs = env.client.run('get-runs')
    runs = json.loads(runs)['result']
    assert len(runs) == numof_runs

    for run in runs:
        id = run['id']
        ec = run['exit_code']

        assert repo.run_exit_code_matches(ec), f"Exit code doesn't match: {ec}"
        # Not implemented yet:
        assert 'status' not in run
        assert 'output' not in run


@my_parametrize('runs_per_client', [1, 5])
@my_parametrize('numof_clients', [1, 5])
def test_repo(env, test_repo, numof_clients, runs_per_client):
    _test_repo_internal(env, test_repo, numof_clients, runs_per_client)


@pytest.mark.stress
@my_parametrize('numof_clients,runs_per_client',
                [
                    (10, 50),
                    (1, 2000),
                    (4, 500),
                ])
def test_repo_stress(env, stress_test_repo, numof_clients, runs_per_client):
    _test_repo_internal(env, stress_test_repo, numof_clients, runs_per_client)


# Nice workaround to skip tests by default: https://stackoverflow.com/a/43938191
@pytest.mark.skipif("not config.getoption('flamegraph')")
@pytest.mark.flame_graph
def test_repo_flame_graph(env, profiler, flame_graph_repo):
    _test_repo_internal(env, flame_graph_repo, 4, 500)
