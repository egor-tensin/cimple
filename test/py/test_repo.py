# Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
# This file is part of the "cimple" project.
# For details, see https://github.com/egor-tensin/cimple.
# Distributed under the MIT License.

from multiprocessing import Process
import re

import pytest

from lib.process import LoggingEvent


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


def _test_repo_internal(env, repo, numof_processes, runs_per_process):
    numof_runs = numof_processes * runs_per_process

    event = LoggingEventRunComplete(numof_runs)
    # Count the number of times the server receives the "run complete" message.
    env.server.logger.add_event(event)

    def client_runner():
        for i in range(runs_per_process):
            env.client.run('run', repo.path, 'HEAD')

    processes = [Process(target=client_runner) for i in range(numof_processes)]
    for proc in processes:
        proc.start()

    event.wait()

    for proc in processes:
        proc.join()

    assert numof_runs == repo.count_ci_output_files()

    runs = env.db.get_all_runs()
    assert numof_runs == len(runs)

    for id, status, ec, output, url, rev in runs:
        assert status == 'finished', f'Invalid status for run {id}: {status}'


@pytest.mark.parametrize('numof_clients,runs_per_client',
                         [(1, 1), (1, 2), (1, 10), (2, 1), (2, 10), (10, 1), (10, 10)])
def test_repo(env, test_repo, numof_clients, runs_per_client):
    _test_repo_internal(env, test_repo, numof_clients, runs_per_client)


@pytest.mark.stress
@pytest.mark.parametrize('numof_clients,runs_per_client',
                         [(1, 2000), (4, 500)])
def test_repo_stress(env, test_repo, numof_clients, runs_per_client):
    _test_repo_internal(env, test_repo, numof_clients, runs_per_client)
