find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_test(
    NAME integration_tests
    COMMAND Python3::Interpreter -m pytest
        "${CMAKE_CURRENT_SOURCE_DIR}"
        --server-binary "$<TARGET_FILE:server>"
        --worker-binary "$<TARGET_FILE:worker>"
        --client-binary "$<TARGET_FILE:client>"
        --project-version "${PROJECT_VERSION}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
