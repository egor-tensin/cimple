add_subdirectory(sigsegv)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(python_test_args
    --no-header -v
    --durations=0 --durations-min=1.0
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

add_python_tests(python_tests_default
    Python3::Interpreter -m pytest ${python_test_args} -m "not flame_graph")

# A subset of tests, excluding long-running stress tests.
add_python_tests(python_tests_sanity
    Python3::Interpreter -m pytest ${python_test_args} -m "not flame_graph and not stress")

# Same, but run under Valgrind.
add_python_tests(python_tests_valgrind
    Python3::Interpreter -m pytest ${python_test_args} -m "not flame_graph and not stress"
        --valgrind "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/valgrind.sh")

if(NOT DEFINED FLAME_GRAPHS_DIR)
    set(FLAME_GRAPHS_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

add_python_tests(python_tests_flame_graphs
    Python3::Interpreter -m pytest ${python_test_args} -m "flame_graph"
        --flamegraph "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/flamegraph.sh"
        --flame-graphs-dir "${FLAME_GRAPHS_DIR}")
