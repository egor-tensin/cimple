add_compile_options(-std=c17 -Wpedantic -Wall -Wextra $<$<NOT:$<CONFIG:Debug>>:-Werror>)

# Even w/ _POSIX_C_SOURCE=200809L, we still need accept4, pipe2, execvpe, etc.,
# which are all _GNU_SOURCE-only (at least for glibc). Try compiling without
# any features macros defined at all, and prepare to be amazed!
add_compile_definitions(_GNU_SOURCE)

# Valgrind on Ubuntu Focal doesn't like DWARF version 5 debug info generated
# by Clang 14.
if("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-fdebug-default-version=4)
endif()

execute_process(
    COMMAND git rev-parse --verify HEAD
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE PROJECT_REV
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)

set(DEFAULT_HOST "127.0.0.1" CACHE STRING "Set default --host value")
set(DEFAULT_PORT "5556" CACHE STRING "Set default --port value")
set(DEFAULT_SQLITE_PATH "/var/lib/cimple/cimple.sqlite" CACHE STRING "Set default SQLite database path")

# The input file has a funny extension, because if I use .c.in, CMake for no
# reason thinks that it should use that to build the binaries and slaps me with
# a CMP0115 policy warning.
configure_file(const.c.template const.c ESCAPE_QUOTES @ONLY)

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

function(add_my_executable name)
    list(POP_FRONT ARGV)
    add_executable("${name}" ${ARGV})
    set_target_properties("${name}" PROPERTIES OUTPUT_NAME "${PROJECT_NAME}-${name}")
    install(TARGETS "${name}" RUNTIME DESTINATION bin)
endfunction()

add_my_executable(server server_main.c server.c
    cmd_line.c
    command.c
    const.c
    event_loop.c
    file.c
    log.c
    msg.c
    net.c
    process.c
    protocol.c
    run_queue.c
    signal.c
    sql/sqlite_sql.h
    sqlite.c
    storage.c
    storage_sqlite.c
    string.c
    tcp_server.c
    worker_queue.c)
target_link_libraries(server PRIVATE pthread sqlite3)
target_include_directories(server PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

add_my_executable(client client_main.c client.c
    cmd_line.c
    const.c
    file.c
    log.c
    msg.c
    net.c)

add_my_executable(worker worker_main.c worker.c
    ci.c
    cmd_line.c
    command.c
    const.c
    event_loop.c
    file.c
    git.c
    log.c
    msg.c
    net.c
    process.c
    protocol.c
    run_queue.c
    signal.c
    string.c)
target_link_libraries(worker PRIVATE git2 pthread)
