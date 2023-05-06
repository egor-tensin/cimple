find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(args
    "${CMAKE_CURRENT_SOURCE_DIR}"
    --server-binary "$<TARGET_FILE:server>"
    --worker-binary "$<TARGET_FILE:worker>"
    --client-binary "$<TARGET_FILE:client>"
    --project-version "${PROJECT_VERSION}")

add_test(NAME integration_tests
    COMMAND Python3::Interpreter -m pytest ${args}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

add_test(NAME integration_tests_with_valgrind
    COMMAND Python3::Interpreter -m pytest ${args}
        --valgrind-binary "${CMAKE_CURRENT_SOURCE_DIR}/../src/valgrind.sh"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
