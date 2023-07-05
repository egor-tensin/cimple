find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(python_test_args
    --no-header -v
    "${CMAKE_CURRENT_SOURCE_DIR}/py"
    --server-binary "$<TARGET_FILE:server>"
    --worker-binary "$<TARGET_FILE:worker>"
    --client-binary "$<TARGET_FILE:client>"
    --project-version "${PROJECT_VERSION}")

function(add_python_tests name)
    list(POP_FRONT ARGV)
    add_test(NAME "${name}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/py"
        COMMAND ${ARGV})
    set_tests_properties("${name}" PROPERTIES TIMEOUT 300)
endfunction()

add_python_tests(python_tests_sanity
    Python3::Interpreter -m pytest ${python_test_args} -m "not stress")

add_python_tests(python_tests_stress
    Python3::Interpreter -m pytest ${python_test_args} -m "stress")

add_python_tests(python_tests_valgrind
    Python3::Interpreter -m pytest ${python_test_args} -m "not stress"
        --valgrind-binary "${CMAKE_CURRENT_SOURCE_DIR}/../src/valgrind.sh")
