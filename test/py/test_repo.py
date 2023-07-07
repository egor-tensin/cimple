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
        super().__init__(timeout=60)

    def log_line_matches(self, line):
        return bool(self.re.search(line))

    def set(self):
        self.counter += 1
        if self.counter == self.target:
            super().set()


def _test_repo_internal(server_and_workers, test_repo, client, numof_processes, runs_per_process, db):
    numof_runs = numof_processes * runs_per_process

    server, workers = server_and_workers

    event = LoggingEventRunComplete(numof_runs)
    # Count the number of times the server receives the "run complete" message.
    server.logger.add_event(event)

    def client_runner():
        for i in range(runs_per_process):
            client.run('run', test_repo.path, 'HEAD')

    processes = [Process(target=client_runner) for i in range(numof_processes)]
    for proc in processes:
        proc.start()
    for proc in processes:
        proc.join()

    event.wait()
    assert numof_runs == test_repo.count_ci_output_files()

    runs = db.get_all_runs()
    assert numof_runs == len(runs)

    for id, status, ec, output, url, rev in runs:
        assert status == 'finished', f'Invalid status for run {id}: {status}'


def test_repo_1_client_1_run(server_and_workers, test_repo, client, sqlite_db):
    _test_repo_internal(server_and_workers, test_repo, client, 1, 1, sqlite_db)


def test_repo_1_client_2_runs(server_and_workers, test_repo, client, sqlite_db):
    _test_repo_internal(server_and_workers, test_repo, client, 1, 2, sqlite_db)


def test_repo_1_client_10_runs(server_and_workers, test_repo, client, sqlite_db):
    _test_repo_internal(server_and_workers, test_repo, client, 1, 10, sqlite_db)


@pytest.mark.stress
def test_repo_1_client_2000_runs(server_and_workers, test_repo, client, sqlite_db):
    _test_repo_internal(server_and_workers, test_repo, client, 1, 2000, sqlite_db)


@pytest.mark.stress
def test_repo_4_clients_500_runs(server_and_workers, test_repo, client, sqlite_db):
    _test_repo_internal(server_and_workers, test_repo, client, 4, 500, sqlite_db)
