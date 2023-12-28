# Copyright (c) 2023 Egor Tensin <egor@tensin.name>
# This file is part of the "cimple" project.
# For details, see https://github.com/egor-tensin/cimple.
# Distributed under the MIT License.

import pytest


# Reference: https://github.com/pytest-dev/pytest/issues/3628
# Automatic generation of readable test IDs.
def my_parametrize(names, values, ids=None, **kwargs):
    _names = names.split(',') if isinstance(names, str) else names
    if not ids:
        if len(_names) == 1:
            ids = [f'{names}={v}' for v in values]
        else:
            _values = [combination.values if hasattr(combination, 'values') else combination
                       for combination in values]
            ids = [
                '-'.join(f'{k}={v}' for k, v in zip(_names, combination))
                for combination in _values
            ]
    return pytest.mark.parametrize(names, values, ids=ids, **kwargs)
