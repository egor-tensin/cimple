cmake_minimum_required(VERSION 3.19) # for COMMAND_ERROR_IS_FATAL

project(cimple VERSION 0.0.1 LANGUAGES C)

enable_testing()

add_compile_options(-std=c17 -Wpedantic -Wall -Wextra)
add_compile_options($<$<NOT:$<CONFIG:Debug>>:-Werror>)

add_compile_options(-fno-omit-frame-pointer -mno-omit-leaf-frame-pointer)

add_subdirectory(src)
add_subdirectory(test)

install(FILES LICENSE.txt DESTINATION "share/doc/${PROJECT_NAME}")
