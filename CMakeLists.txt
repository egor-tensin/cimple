cmake_minimum_required(VERSION 3.3) # for include-what-you-use

project(cimple VERSION 0.0.1 LANGUAGES C)

add_subdirectory(src)

install(FILES LICENSE.txt DESTINATION "share/doc/${PROJECT_NAME}")
