# Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
# This file is part of the "cimple" project.
# For details, see https://github.com/egor-tensin/cimple.
# Distributed under the MIT License.

from lib.process import LoggingEvent


class LoggingEventRunComplete(LoggingEvent):
    def __init__(self, target):
        self.counter = 0
        self.target = target
        super().__init__(timeout=60)

    def log_line_matches(self, line):
        return 'Received a "run complete" message from worker' in line

    def set(self):
        self.counter += 1
        if self.counter == self.target:
            super().set()


def _test_repo_internal(server_and_workers, test_repo, client, numof_runs):
    server, workers = server_and_workers

    event = LoggingEventRunComplete(numof_runs)
    # Count the number of times the server receives the "run complete" message.
    server.logger.add_event(event)

    for i in range(numof_runs):
        client.run('run', test_repo.path, 'HEAD')

    event.wait()
    assert numof_runs == test_repo.count_ci_output_files()


def test_repo(server_and_workers, test_repo, client):
    _test_repo_internal(server_and_workers, test_repo, client, 1)


def test_repo_2(server_and_workers, test_repo, client):
    _test_repo_internal(server_and_workers, test_repo, client, 2)


def test_repo_10(server_and_workers, test_repo, client):
    _test_repo_internal(server_and_workers, test_repo, client, 10)
