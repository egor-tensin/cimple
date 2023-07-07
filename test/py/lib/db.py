# Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
# This file is part of the "cimple" project.
# For details, see https://github.com/egor-tensin/cimple.
# Distributed under the MIT License.

from contextlib import closing, contextmanager
import sqlite3


class Database:
    def __init__(self, path):
        self.conn = sqlite3.connect(f'file:{path}?mode=ro')

    def __enter__(self):
        return self

    def __exit__(*args):
        self.conn.close()

    @contextmanager
    def get_cursor(self):
        with closing(self.conn.cursor()) as cur:
            yield cur

    def get_all_runs(self):
        with self.get_cursor() as cur:
            cur.execute('SELECT * FROM cimple_runs_readable')
            return cur.fetchall()
