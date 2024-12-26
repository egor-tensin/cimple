### Setting up

Set up git hooks by running

    ./scripts/setup-hooks.sh

### Building

There's a Makefile with useful shortcuts to build the project in the "build/"
directory:

    make debug   # Debug build in build/debug/cmake
    make release # Release build in build/release/cmake

This command makes a CMake build directory in build/{debug,release}/cmake/ and
executes `make` there.

The default is to build using Clang.
You can choose a different compiler like so:

    make {debug,release} COMPILER=gcc

### Testing

After building, you can run the "test suite" (depends on Pytest).

    make debug/test
    make release/test

To only run a subset of basic sanity tests:

    make debug/sanity
    make release/sanity

To generate an HTML report in build/{debug,release}/test_report/, run (requires
pytest-html):

    make debug/report
    make release/report

Reports for the latest successful Clang builds can be found below:

* [debug],
* [release].

[debug]: https://egor-tensin.github.io/cimple/test_report_clang_debug/
[release]: https://egor-tensin.github.io/cimple/test_report_clang_release/

### Valgrind

You can run a suite of basic sanity tests under Valgrind:

    make debug/valgrind
    make release/valgrind

### Code coverage

You can generate a code coverage report (depends on `gcovr`) in
build/coverage/html:

    make coverage

The latest code coverage report for the `master` branch can be found at
https://egor-tensin.github.io/cimple/coverage/.

### Static analysis

Build w/ GCC's `-fanalyzer`:

    make analyzer

### Flame graphs

Some performance analysis can be done by looking at flame graphs.
Generate them after building the project (depends on `perf` & [FlameGraph]):

    make debug/flame_graphs

[FlameGraph]: https://github.com/brendangregg/FlameGraph

This will generate two flame graphs in build/debug/flame_graphs/; they stress
slightly different parts of the system:

* [flame_graph_output_simple.svg] for a CI script with short output,
* [flame_graph_output_long.svg] for a CI script with long output.

[flame_graph_output_simple.svg]: https://egor-tensin.github.io/cimple/flame_graphs/flame_graph_output_simple.svg
[flame_graph_output_long.svg]: https://egor-tensin.github.io/cimple/flame_graphs/flame_graph_output_long.svg

Follow the links above to view the latest flame graphs for the `master` branch.
