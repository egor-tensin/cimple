# Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
# This file is part of the "cimple" project.
# For details, see https://github.com/egor-tensin/cimple.
# Distributed under the MIT License.

from contextlib import contextmanager
import logging
import os
import shutil
import subprocess
from threading import Event, Lock, Thread


class LoggingEvent(Event):
    def __init__(self, timeout=10):
        self.timeout = timeout
        super().__init__()

    def log_line_matches(self, line):
        return False

    def wait(self):
        if not super().wait(self.timeout):
            raise RuntimeError('timed out while waiting for an event')


class LoggingThread(Thread):
    def __init__(self, process, events=None):
        self.process = process
        self.events_lock = Lock()
        if events is None:
            events = []
        self.events = events

        super().__init__(target=lambda: self.process_output_lines())
        self.start()

    def add_event(self, event):
        with self.events_lock:
            self.events.append(event)

    def process_output_lines(self):
        for line in self.process.stdout:
            line = line.removesuffix('\n')
            logging.info('%s: %s', self.process.log_id, line)
            with self.events_lock:
                for event in self.events:
                    if event.is_set():
                        continue
                    if not event.log_line_matches(line):
                        continue
                    event.set()
                self.events = [event for event in self.events if not event.is_set()]


class CmdLine:
    @staticmethod
    def which(binary):
        if os.path.split(binary)[0]:
            # shutil.which('bin/bash') doesn't work.
            return os.path.abspath(binary)
        path = shutil.which(binary)
        if path is None:
            raise RuntimeError("couldn't find a binary: " + binary)
        return path

    def __init__(self, binary, *args, name=None):
        binary = self.which(binary)
        argv = [binary] + list(args)

        self.binary = binary
        self.argv = argv

        if name is None:
            name = os.path.basename(binary)
        self.process_name = name

    def log_line_means_process_ready(self, line):
        return True

    @classmethod
    def wrap(cls, outer, inner):
        return cls(outer.argv[0], *outer.argv[1:], *inner.argv, name=inner.process_name)

    def log_process_start(self):
        if len(self.argv) > 1:
            logging.info('Executing binary %s with arguments: %s', self.binary, ' '.join(self.argv[1:]))
        else:
            logging.info('Executing binary %s', self.binary)


class LoggingEventProcessReady(LoggingEvent):
    def __init__(self, process):
        self.process = process
        super().__init__()

    def set(self):
        logging.info('Process %s is ready', self.process.log_id)
        super().set()

    def log_line_matches(self, line):
        return self.process.cmd_line.log_line_means_process_ready(line)


class Process(subprocess.Popen):
    _COMMON_ARGS = {
        'text': True,
        'stdin': subprocess.DEVNULL,
        'stdout': subprocess.PIPE,
        'stderr': subprocess.STDOUT,
    }

    @staticmethod
    def run(*args, **kwargs):
        try:
            return subprocess.run(list(args), check=True, **Process._COMMON_ARGS, **kwargs)
        except subprocess.CalledProcessError as e:
            logging.error('Command %s exited with code %s', e.cmd, e.returncode)
            logging.error('Output:\n%s', e.output)
            raise

    def __init__(self, cmd_line):
        self.cmd_line = cmd_line

        super().__init__(cmd_line.argv, **Process._COMMON_ARGS)
        logging.info('Process %s has started', self.log_id)

        ready_event = LoggingEventProcessReady(self)
        self.logger = LoggingThread(self, [ready_event])
        ready_event.wait()

    @property
    def log_id(self):
        return f'{self.pid}/{self.cmd_line.process_name}'

    def __exit__(self, *args):
        try:
            self.shut_down()
            self.logger.join()
        except Exception as e:
            logging.exception(e)
        # Postpone closing the pipes until after the logging thread is finished
        # so that it doesn't attempt to read from closed descriptors.
        super().__exit__(*args)

    SHUT_DOWN_TIMEOUT_SEC = 3

    def shut_down(self):
        ec = self.poll()
        if ec is not None:
            return
        logging.info('Terminating process %s', self.log_id)
        self.terminate()
        try:
            self.wait(timeout=Process.SHUT_DOWN_TIMEOUT_SEC)
            return
        except subprocess.TimeoutExpired:
            pass
        logging.info('Process %s failed to terminate in time, killing it', self.log_id)
        self.kill()
        self.wait(timeout=Process.SHUT_DOWN_TIMEOUT_SEC)


class Runner:
    @staticmethod
    def unbuffered():
        return CmdLine('stdbuf', '-o0')

    def __init__(self):
        self.wrappers = []
        self.add_wrapper(self.unbuffered())

    def add_wrapper(self, cmd_line):
        self.wrappers.append(cmd_line)

    def _wrap(self, cmd_line):
        for wrapper in self.wrappers:
            cmd_line = cmd_line.wrap(wrapper, cmd_line)
        return cmd_line

    def run(self, cmd_line):
        cmd_line = self._wrap(cmd_line)
        cmd_line.log_process_start()

        result = Process.run(*cmd_line.argv)
        return result.returncode, result.stdout

    @contextmanager
    def run_async(self, cmd_line):
        cmd_line = self._wrap(cmd_line)
        cmd_line.log_process_start()

        with Process(cmd_line) as process:
            yield process


class CmdLineRunner:
    def __init__(self, runner, binary, *args):
        self.runner = runner
        self.binary = binary
        self.args = list(args)

    def _build(self, *args):
        return CmdLine(self.binary, *self.args, *args)

    def run(self, *args):
        return self.runner.run(self._build(*args))

    def run_async(self, *args):
        return self.runner.run_async(self._build(*args))
