cmake_minimum_required(VERSION 3.0)

project(cimple VERSION 0.0.1 LANGUAGES C)

add_subdirectory(src)

install(FILES LICENSE.txt DESTINATION "share/doc/${PROJECT_NAME}")
