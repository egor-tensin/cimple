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


CI_SCRIPT = R'''#!/usr/bin/env bash

set -o errexit -o nounset -o pipefail
shopt -s inherit_errexit lastpipe

readonly runs_dir={runs_dir}

readonly run_output_template=run_XXXXXX

run_output_path="$( mktemp --tmpdir="$runs_dir" "$run_output_template" )"
readonly run_output_path

touch -- "$run_output_path"
'''


class TestRepo(Repo):
    # Prevent Pytest from discovering test cases in this class:
    __test__ = False

    def _format_ci_script(self):
        runs_dir = shlex.quote(self.runs_dir)
        return CI_SCRIPT.format(runs_dir=runs_dir)

    def __init__(self, path, ci_script='ci.sh'):
        super().__init__(path)

        self.runs_dir = os.path.join(self.path, 'runs')
        os.makedirs(self.runs_dir, exist_ok=True)

        self.ci_script_path = os.path.join(self.path, ci_script)
        with open(self.ci_script_path, mode='x') as f:
            f.write(self._format_ci_script())
        os.chmod(self.ci_script_path, 0o755)

        self.run('git', 'add', '--', ci_script)
        self.run('git', 'commit', '-q', '-m', 'add CI script')

    def count_run_files(self):
        return len([name for name in os.listdir(self.runs_dir) if os.path.isfile(os.path.join(self.runs_dir, name))])


class TestRepoOutput(TestRepo, abc.ABC):
    __test__ = False

    OUTPUT_SCRIPT_NAME = 'generate-output'

    def __init__(self, path):
        super().__init__(path)

        self.output_script_path = os.path.join(self.path, TestRepoOutput.OUTPUT_SCRIPT_NAME)
        with open(self.output_script_path, mode='x') as f:
            f.write(self._format_output_script())
        os.chmod(self.output_script_path, 0o755)

        with open(self.ci_script_path, mode='a') as f:
            f.write(self._format_ci_script_addition())

        self.run('git', 'add', '--', TestRepoOutput.OUTPUT_SCRIPT_NAME)
        self.run('git', 'add', '-u')
        self.run('git', 'commit', '-q', '-m', 'add output script')

    @abc.abstractmethod
    def _format_output_script(self):
        pass

    def _format_ci_script_addition(self):
        return R'{output_script} | tee -a "$run_output_path"'.format(
            output_script=shlex.quote(self.output_script_path))

    @abc.abstractmethod
    def run_output_matches(self, output):
        pass


OUTPUT_SCRIPT_SIMPLE = R'''#!/usr/bin/env bash

set -o errexit -o nounset -o pipefail
shopt -s inherit_errexit lastpipe

timestamp="$( date --iso-8601=ns )"
readonly timestamp

echo "A CI run happened at $timestamp"
'''


class TestRepoOutputSimple(TestRepoOutput):
    __test__ = False

    def _format_output_script(self):
        return OUTPUT_SCRIPT_SIMPLE

    def run_output_matches(self, output):
        return output.decode().startswith('A CI run happened at ')


OUTPUT_SCRIPT_EMPTY = R'''#!/bin/sh

true
'''


class TestRepoOutputEmpty(TestRepoOutput):
    __test__ = False

    def _format_output_script(self):
        return OUTPUT_SCRIPT_EMPTY

    def run_output_matches(self, output):
        return len(output) == 0


# Making it a bash script introduces way too much overhead with all the
# argument expansions; it slows things down considerably.
OUTPUT_SCRIPT_LONG = R'''#!/usr/bin/env python3

output = {output}

import sys
sys.stdout.write(output)
'''


class TestRepoOutputLong(TestRepoOutput):
    __test__ = False

    OUTPUT_LEN_KB = 300

    def __init__(self, *args, **kwargs):
        nb = TestRepoOutputLong.OUTPUT_LEN_KB * 1024
        self.output = base64.encodebytes(random.randbytes(nb)).decode()
        super().__init__(*args, **kwargs)

    def _format_output_script(self):
        return OUTPUT_SCRIPT_LONG.format(output=repr(self.output))

    def run_output_matches(self, output):
        return output.decode() == self.output


OUTPUT_SCRIPT_NULL = R'''#!/usr/bin/env python3

output = {output}

import sys
sys.stdout.buffer.write(output)
'''


class TestRepoOutputNull(TestRepoOutput):
    __test__ = False

    OUTPUT = b'123\x00456'

    def __init__(self, *args, **kwargs):
        self.output = TestRepoOutputNull.OUTPUT
        super().__init__(*args, **kwargs)

    def _format_output_script(self):
        assert len(self.output) == 7
        return OUTPUT_SCRIPT_NULL.format(output=repr(self.output))

    def run_output_matches(self, output):
        return output == self.output
