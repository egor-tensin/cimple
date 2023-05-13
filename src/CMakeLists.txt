if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror)
endif()

add_compile_definitions(_GNU_SOURCE)

add_compile_definitions(VERSION="${PROJECT_VERSION}")

set(DEFAULT_HOST "127.0.0.1" CACHE STRING "Set the default --host value")
add_compile_definitions(DEFAULT_HOST="${DEFAULT_HOST}")

function(add_my_executable name)
    list(POP_FRONT ARGV)
    add_executable("${name}" ${ARGV})
    set_target_properties("${name}" PROPERTIES OUTPUT_NAME "${PROJECT_NAME}-${name}")
    install(TARGETS "${name}" RUNTIME DESTINATION bin)
endfunction()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

function(generate_sql_header engine)
    file(GLOB sql_files "${CMAKE_CURRENT_SOURCE_DIR}/${engine}/*.sql")
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sql/${engine}_sql.h"
        COMMAND Python3::Interpreter
            "${CMAKE_CURRENT_SOURCE_DIR}/generate-sql-header.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/${engine}/"
            -o "${CMAKE_CURRENT_BINARY_DIR}/sql/${engine}_sql.h"
        DEPENDS ${sql_files})
endfunction()

generate_sql_header(sqlite)

add_my_executable(server server_main.c server.c
    ci_queue.c
    cmd_line.c
    command.c
    file.c
    msg.c
    net.c
    signal.c
    sql/sqlite_sql.h
    sqlite.c
    storage.c
    storage_sqlite.c
    tcp_server.c)
target_link_libraries(server PRIVATE pthread sqlite3)
target_include_directories(server PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

add_my_executable(client client_main.c client.c
    cmd_line.c
    file.c
    msg.c
    net.c)

add_my_executable(worker worker_main.c worker.c
    ci.c
    cmd_line.c
    command.c
    file.c
    git.c
    msg.c
    net.c
    process.c
    signal.c)
target_link_libraries(worker PRIVATE git2 pthread)
