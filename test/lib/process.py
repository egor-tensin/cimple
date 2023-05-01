# Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
# This file is part of the "cimple" project.
# For details, see https://github.com/egor-tensin/cimple.
# Distributed under the MIT License.

from contextlib import contextmanager
import logging
import os
import subprocess
from threading import Thread
import time


_COMMON_ARGS = {
    'text': True,
    'stdin': subprocess.DEVNULL,
    'stdout': subprocess.PIPE,
    'stderr': subprocess.STDOUT,
}


def _canonicalize_process_args(binary, *args):
    binary = os.path.abspath(binary)
    argv = list(args)
    argv = [binary] + argv
    return binary, argv


def _log_process_args(binary, argv):
    if argv:
        logging.info('Executing binary %s with arguments: %s', binary, ' '.join(argv[1:]))
    else:
        logging.info('Executing binary %s', binary)


class LoggingThread(Thread):
    def __init__(self, process):
        self.process = process
        target = lambda pipe: self.consume(pipe)
        super().__init__(target=target, args=[process.stdout])

    def consume(self, pipe):
        for line in iter(pipe):
            logging.info('%s: %s', self.process.log_id, line)

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, *args):
        self.process.shut_down()
        self.join()


class Process(subprocess.Popen):
    def __init__(self, binary, *args):
        binary, argv = _canonicalize_process_args(binary, *args)
        _log_process_args(binary, argv)

        self.binary = binary
        self.name = os.path.basename(binary)

        super().__init__(argv, **_COMMON_ARGS)
        # TODO: figure out how to remove this.
        time.sleep(1)
        logging.info('Process %s launched', self.log_id)

    @property
    def log_id(self):
        return f'{self.pid}/{self.name}'

    def __exit__(self, *args):
        try:
            self.shut_down()
        finally:
            super().__exit__(*args)

    def shut_down(self):
        ec = self.poll()
        if ec is not None:
            return
        logging.info('Terminating process %s', self.log_id)
        self.terminate()
        try:
            self.wait(timeout=3)
            return
        except subprocess.TimeoutExpired:
            pass
        self.kill()
        self.wait(timeout=3)


@contextmanager
def run_async(binary, *args):
    with Process(binary, *args) as process, \
         LoggingThread(process):
        yield process


def run(binary, *args):
    binary, argv = _canonicalize_process_args(binary, *args)
    _log_process_args(binary, argv)
    result = subprocess.run(argv, **_COMMON_ARGS)
    return result.returncode, result.stdout
