cmake_minimum_required(VERSION 3.1)

PROJECT(${CMAKE_PROJECT_NAME}_3rdparty)

add_subdirectory(googletest)
add_subdirectory(spdlog)
add_subdirectory(libcoroutine)