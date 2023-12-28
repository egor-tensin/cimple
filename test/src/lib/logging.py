# Copyright (c) 2023 Egor Tensin <egor@tensin.name>
# This file is part of the "cimple" project.
# For details, see https://github.com/egor-tensin/cimple.
# Distributed under the MIT License.

from contextlib import contextmanager
import logging
import logging.config
import logging.handlers
import multiprocessing as mp


@contextmanager
def child_logging_thread():
    # Delegating logging to the parent logger.
    ctx = mp.get_context('spawn')
    queue = ctx.Queue()
    listener = logging.handlers.QueueListener(queue, logging.getLogger())
    listener.start()
    try:
        yield queue
    finally:
        listener.stop()


@contextmanager
def configure_logging_in_child(queue):
    config = {
        'version': 1,
        'handlers': {
            'sink': {
                'class': 'logging.handlers.QueueHandler',
                'queue': queue,
            },
        },
        'root': {
            'handlers': ['sink'],
            'level': 'DEBUG',
        },
    }
    logging.config.dictConfig(config)
    try:
        yield
    except Exception as e:
        logging.exception(e)
