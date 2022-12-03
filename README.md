cimple
======

My little CI system (hopefully).

_This is work in progress; it doesn't quite work the way I want it to yet._

Development
-----------

Build using CMake.
Depends on libgit2 and SQLite.

There's a Makefile with useful shortcuts to build the project in the .build/
directory:

    make build

### Code style

Set up the git pre-commit hook by running `./scripts/setup-hooks.sh`.
This depends on `clang-format` and won't allow you to commit code that doesn't
pass the formatting check.

Rationale
---------

The goal it to make a CI system that doesn't suck.
See [rationale.md] for details.

[rationale.md]: doc/rationale.md

License
-------

Distributed under the MIT License.
See [LICENSE.txt] for details.

[LICENSE.txt]: LICENSE.txt
