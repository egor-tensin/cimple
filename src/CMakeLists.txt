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

add_my_executable(server server_main.c server.c
    ci_queue.c
    msg.c
    net.c
    signal.c
    storage.c
    storage_sqlite.c
    tcp_server.c)
target_link_libraries(server PRIVATE pthread sqlite3)

add_my_executable(client client_main.c client.c
    msg.c
    net.c)

add_my_executable(worker worker_main.c worker.c
    ci.c
    file.c
    git.c
    msg.c
    net.c
    process.c
    signal.c)
target_link_libraries(worker PRIVATE git2 pthread)
