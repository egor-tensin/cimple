#!/usr/bin/env python3

# Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
# This file is part of the "cimple" project.
# For details, see https://github.com/egor-tensin/cimple.
# Distributed under the MIT License.

import argparse
from contextlib import contextmanager
from glob import glob
import os
import sys


class Generator:
    def __init__(self, fd, dir):
        self.fd = fd
        if not os.path.isdir(dir):
            raise RuntimeError('must be a directory: ' + dir)
        self.dir = os.path.abspath(dir)
        self.name = os.path.basename(self.dir)

    def write(self, line):
        self.fd.write(f'{line}\n')

    def do(self):
        self.include_guard_start()
        self.include_sql_files()
        self.include_guard_end()

    def include_guard_start(self):
        self.write(f'#ifndef __{self.name.upper()}_SQL_H__')
        self.write(f'#define __{self.name.upper()}_SQL_H__')
        self.write('')

    def include_guard_end(self):
        self.write('')
        self.write(f'#endif')

    def enum_sql_files(self):
        return [os.path.join(self.dir, path) for path in sorted(glob('*.sql', root_dir=self.dir))]

    @property
    def var_name_prefix(self):
        return f'sql_{self.name}'

    def sql_file_to_var_name(self, path):
        name = os.path.splitext(os.path.basename(path))[0]
        return f'{self.var_name_prefix}_{name}'

    @staticmethod
    def sql_file_to_string_literal(path):
        with open(path) as fd:
            sql = fd.read()
        sql = sql.encode().hex().upper()
        sql = ''.join((f'\\x{sql[i:i + 2]}' for i in range(0, len(sql), 2)))
        return sql

    def include_sql_files(self):
        vars = []
        for path in self.enum_sql_files():
            name = self.sql_file_to_var_name(path)
            vars.append(name)
            value = self.sql_file_to_string_literal(path)
            self.write(f'static const char *const {name} = "{value}";')
        self.write('')
        self.write(f'static const char *const {self.var_name_prefix}_files[] = {{')
        for var in vars:
            self.write(f'\t{var},')
        self.write('};')


@contextmanager
def open_output(path):
    if path is None:
        yield sys.stdout
    else:
        with open(path, 'w') as fd:
            yield fd


def parse_args(argv=None):
    if argv is None:
        argv = sys.argv[1:]
    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', metavar='PATH',
                        help='set output file path')
    parser.add_argument('dir', metavar='INPUT_DIR',
                        help='input directory')
    return parser.parse_args()


def main(argv=None):
    args = parse_args(argv)
    with open_output(args.output) as fd:
        generator = Generator(fd, args.dir)
        generator.do()


if __name__ == '__main__':
    main()
