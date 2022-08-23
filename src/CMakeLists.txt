cmake_minimum_required(VERSION 3.0)

project(tinyCI VERSION 0.0.1 LANGUAGES C)

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror)
endif()

add_compile_definitions(_GNU_SOURCE)

add_compile_definitions(VERSION="${PROJECT_VERSION}")

add_executable(server server_main.c server.c msg.c net.c)
add_executable(client client_main.c client.c msg.c net.c)
