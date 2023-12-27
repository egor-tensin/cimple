add_subdirectory(sigsegv)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

if(NOT DEFINED TEST_REPORT_DIR)
    set(TEST_REPORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if(NOT DEFINED FLAME_GRAPHS_DIR)
    set(FLAME_GRAPHS_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

set(python_test_args
    --no-header -v
    --durations 0 --durations-min 1.0
    "${CMAKE_CURRENT_SOURCE_DIR}/py"
    --server "$<TARGET_FILE:server>"
    --worker "$<TARGET_FILE:worker>"
    --client "$<TARGET_FILE:client>"
    --sigsegv "$<TARGET_FILE:sigsegv>"
    --project-version "${PROJECT_VERSION}")

function(add_python_tests name)
    list(POP_FRONT ARGV)
    add_test(NAME "${name}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/py"
        COMMAND ${ARGV})
    set_tests_properties("${name}" PROPERTIES TIMEOUT 600)
endfunction()

# Fast sanity tests & longer-running stress tests.
add_python_tests(python_tests_default
    Python3::Interpreter -m pytest ${python_test_args})

# Same, but use pytest-html to generate an HTML report.
add_python_tests(python_tests_report
    Python3::Interpreter -m pytest ${python_test_args}
        --html "${TEST_REPORT_DIR}/index.html")

# Fast sanity tests only.
add_python_tests(python_tests_sanity
    Python3::Interpreter -m pytest ${python_test_args} -m "not stress")

# Same, but run under Valgrind. Longer-running stress tests are not run under
# Valgrind, because they take too long.
add_python_tests(python_tests_valgrind
    Python3::Interpreter -m pytest ${python_test_args} -m "not stress"
        --valgrind "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/valgrind.sh")

# A couple of tests that are profiled using the flame graph tool. These are
# excluded by default.
add_python_tests(python_tests_flame_graphs
    Python3::Interpreter -m pytest ${python_test_args} -m "flame_graph"
        --flamegraph "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/flamegraph.sh"
        --flame-graphs-dir "${FLAME_GRAPHS_DIR}")
