There's a Makefile with useful shortcuts to build the project in the "build/"
directory:

    make build

This command makes a CMake build directory in build/cmake/ and executes `make`
there.

The default is to build using clang in `Debug` configuration.
You can choose a different compiler and configuration like so:

    make build COMPILER=gcc CONFIGURATION=Release

### Testing

After building, you can run the "test suite" (depends on Pytest).

    make test

To only run a subset of basic sanity tests:

    make test/sanity

### Valgrind

You can run a suite of basic sanity tests under Valgrind:

    make test/valgrind

### Code coverage

You can generate a code coverage report (depends on `gcovr`) in
build/coverage/:

    make coverage

The latest code coverage report for the `master` branch can be found at
https://egor-tensin.github.io/cimple/coverage/.

### Flame graphs

Some performance analysis can be done by looking at flame graphs.
Generate them after building the project (depends on `perf` & [FlameGraph]):

    make flame_graphs

[FlameGraph]: https://github.com/brendangregg/FlameGraph

This will generate two flame graphs in build/flame_graphs/; they stress
slightly different parts of the system:

* [flame_graph_output_simple.svg] for a CI script with short output,
* [flame_graph_output_long.svg] for a CI script with long output.

[flame_graph_output_simple.svg]: https://egor-tensin.github.io/cimple/flame_graphs/flame_graph_output_simple.svg
[flame_graph_output_long.svg]: https://egor-tensin.github.io/cimple/flame_graphs/flame_graph_output_long.svg

Follow the links above to view the latest flame graphs for the `master` branch.
