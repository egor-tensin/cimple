# Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
# This file is part of the "cimple" project.
# For details, see https://github.com/egor-tensin/cimple.
# Distributed under the MIT License.

import logging
import os
import shutil

from .process import Process


class Repo:
    BRANCH = 'main'

    def __init__(self, path):
        self.path = os.path.abspath(path)
        os.makedirs(path, exist_ok=True)
        self.run('git', 'init', f'--initial-branch={Repo.BRANCH}')
        self.run('git', 'config', 'user.name', 'Test User')
        self.run('git', 'config', 'user.email', 'test@example.com')

    def run(self, *args, **kwargs):
        Process.run(*args, cwd=self.path, **kwargs)


class TestRepo(Repo):
    # Prevent Pytest from discovering test cases in this class:
    __test__ = False

    DATA_DIR = 'test_repo'
    CI_SCRIPT = 'ci.sh'
    OUTPUT_DIR = 'output'

    @staticmethod
    def get_ci_script():
        return os.path.join(os.path.dirname(__file__), TestRepo.DATA_DIR, TestRepo.CI_SCRIPT)

    def __init__(self, path):
        super().__init__(path)
        shutil.copy(self.get_ci_script(), self.path)
        self.run('git', 'add', '.')
        self.run('git', 'commit', '-m', 'add CI script')
        self.output_dir = os.path.join(self.path, TestRepo.OUTPUT_DIR)
        os.makedirs(self.output_dir, exist_ok=True)

    def count_ci_output_files(self):
        return len([name for name in os.listdir(self.output_dir) if os.path.isfile(os.path.join(self.output_dir, name))])
