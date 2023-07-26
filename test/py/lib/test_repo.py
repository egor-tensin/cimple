# Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
# This file is part of the "cimple" project.
# For details, see https://github.com/egor-tensin/cimple.
# Distributed under the MIT License.

import abc
import base64
import logging
import os
import random
import shlex
import shutil

from .process import Process


class Repo:
    BRANCH = 'main'

    def __init__(self, path):
        self.path = os.path.abspath(path)
        os.makedirs(path, exist_ok=True)
        self.run('git', 'init', '-q', f'--initial-branch={Repo.BRANCH}')
        self.run('git', 'config', 'user.name', 'Test User')
        self.run('git', 'config', 'user.email', 'test@example.com')

    def run(self, *args, **kwargs):
        Process.run(*args, cwd=self.path, **kwargs)


CI_SCRIPT = r'''#!/bin/sh -e
readonly runs_dir={runs_dir}
readonly run_output_template=run_XXXXXX
run_output_path="$( mktemp --tmpdir="$runs_dir" "$run_output_template" )"
touch -- "$run_output_path"
'''


class TestRepo(Repo):
    # Prevent Pytest from discovering test cases in this class:
    __test__ = False

    def __init__(self, path, ci_script='ci'):
        super().__init__(path)

        self.runs_dir = os.path.join(self.path, 'runs')
        os.makedirs(self.runs_dir, exist_ok=True)

        self.ci_script_path = os.path.join(self.path, ci_script)

        self.write_ci_script()
        self.run('git', 'add', '--', ci_script)
        self.run('git', 'commit', '-q', '-m', 'add CI script')

    @staticmethod
    @abc.abstractmethod
    def codename():
        pass

    @abc.abstractmethod
    def run_exit_code_matches(self, ec):
        pass

    @abc.abstractmethod
    def run_output_matches(self, output):
        pass

    def write_ci_script(self):
        with open(self.ci_script_path, mode='x') as f:
            f.write(self.format_ci_script())
        os.chmod(self.ci_script_path, 0o755)

    def format_ci_script(self):
        runs_dir = shlex.quote(self.runs_dir)
        return CI_SCRIPT.format(runs_dir=runs_dir)

    def _count_run_files(self):
        return len([name for name in os.listdir(self.runs_dir) if os.path.isfile(os.path.join(self.runs_dir, name))])

    def run_files_are_present(self, expected):
        assert expected == self._count_run_files()


class TestRepoOutput(TestRepo, abc.ABC):
    __test__ = False

    OUTPUT_SCRIPT_NAME = 'generate-output'

    def __init__(self, path):
        self.output_script_path = os.path.join(path, TestRepoOutput.OUTPUT_SCRIPT_NAME)
        super().__init__(path)

        self.write_output_script()
        self.run('git', 'add', '--', TestRepoOutput.OUTPUT_SCRIPT_NAME)
        self.run('git', 'commit', '-q', '-m', 'add output script')

    def format_ci_script(self):
        script = super().format_ci_script()
        added = r'{output_script} | tee -a "$run_output_path"'.format(
            output_script=shlex.quote(self.output_script_path))
        return f'{script}\n{added}\n'

    def write_output_script(self):
        with open(self.output_script_path, mode='x') as f:
            f.write(self.format_output_script())
        os.chmod(self.output_script_path, 0o755)

    @abc.abstractmethod
    def format_output_script(self):
        pass

    def run_exit_code_matches(self, ec):
        return ec == 0


OUTPUT_SCRIPT_SIMPLE = r'''#!/bin/sh -e
timestamp="$( date --iso-8601=ns )"
echo "A CI run happened at $timestamp"
'''


class TestRepoOutputSimple(TestRepoOutput):
    __test__ = False

    @staticmethod
    def codename():
        return 'output_simple'

    def format_output_script(self):
        return OUTPUT_SCRIPT_SIMPLE

    def run_output_matches(self, output):
        return output.decode().startswith('A CI run happened at ')


OUTPUT_SCRIPT_EMPTY = r'''#!/bin/sh
'''


class TestRepoOutputEmpty(TestRepoOutput):
    __test__ = False

    @staticmethod
    def codename():
        return 'output_empty'

    def format_output_script(self):
        return OUTPUT_SCRIPT_EMPTY

    def run_output_matches(self, output):
        return len(output) == 0


OUTPUT_SCRIPT_LONG = r'''#!/bin/sh -e
dd if=/dev/urandom count={output_len_kb} bs=1024 | base64
'''


class TestRepoOutputLong(TestRepoOutput):
    __test__ = False

    OUTPUT_LEN_KB = 300

    @staticmethod
    def codename():
        return 'output_long'

    def format_output_script(self):
        output_len = TestRepoOutputLong.OUTPUT_LEN_KB
        output_len = shlex.quote(str(output_len))
        return OUTPUT_SCRIPT_LONG.format(output_len_kb=output_len)

    def run_output_matches(self, output):
        return len(output) > TestRepoOutputLong.OUTPUT_LEN_KB * 1024


OUTPUT_SCRIPT_NULL = r'''#!/usr/bin/env python3
output = {output}
import sys
sys.stdout.buffer.write(output)
'''


class TestRepoOutputNull(TestRepoOutput):
    __test__ = False

    OUTPUT = b'123\x00456\x00789'

    def __init__(self, *args, **kwargs):
        assert len(TestRepoOutputNull.OUTPUT) == 11
        self.output = TestRepoOutputNull.OUTPUT
        super().__init__(*args, **kwargs)

    @staticmethod
    def codename():
        return 'output_null'

    def format_output_script(self):
        return OUTPUT_SCRIPT_NULL.format(output=repr(self.output))

    def run_output_matches(self, output):
        return output == self.output


class TestRepoSegfault(TestRepo):
    __test__ = False

    def __init__(self, repo_path, prog_path):
        self.prog_path = prog_path
        super().__init__(repo_path)

    @staticmethod
    def codename():
        return 'segfault'

    def write_ci_script(self):
        shutil.copy(self.prog_path, self.ci_script_path)

    def run_exit_code_matches(self, ec):
        return ec == -11

    def run_output_matches(self, output):
        return "Started the test program.\n" == output.decode()

    def run_files_are_present(self, *args):
        return True
