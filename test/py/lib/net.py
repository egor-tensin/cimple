# Copyright (c) 2023 Egor Tensin <Egor.Tensin@gmail.com>
# This file is part of the "cimple" project.
# For details, see https://github.com/egor-tensin/cimple.
# Distributed under the MIT License.

from contextlib import closing
import random
import socket


def port_is_open(host, port):
    with closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as sock:
        return sock.connect_ex((host, port)) == 0


def random_unused_port():
    while True:
        port = random.randint(20000, 40000)
        if port_is_open('127.0.0.1', port):
            continue
        return port
