find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(python_test_args
    --no-header -v
    "${CMAKE_CURRENT_SOURCE_DIR}"
    --server-binary "$<TARGET_FILE:server>"
    --worker-binary "$<TARGET_FILE:worker>"
    --client-binary "$<TARGET_FILE:client>"
    --project-version "${PROJECT_VERSION}")

function(add_my_test name)
    list(POP_FRONT ARGV)
    add_test(NAME "${name}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMAND ${ARGV})
    set_tests_properties("${name}" PROPERTIES TIMEOUT 60)
endfunction()

add_my_test(python_tests
    Python3::Interpreter -m pytest ${python_test_args})

add_my_test(python_tests_valgrind
    Python3::Interpreter -m pytest ${python_test_args}
        --valgrind-binary "${CMAKE_CURRENT_SOURCE_DIR}/../src/valgrind.sh")
