cmake_minimum_required(VERSION 3.1)

include(cmake/utils.cmake)
include(cmake/messagecolor.cmake)
tink_extract_version()
project(tink VERSION ${TINK_VERSION} LANGUAGES CXX C)

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose Release or Debug" FORCE)
ENDIF()

MESSAGE(STATUS "Build type: " ${CMAKE_BUILD_TYPE})

MESSAGE( "** INFO ------------------------------------------------")
MESSAGE( "** STATUS Build tink ${TINK_VERSION}")


# 设置可执行文件输出目录
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(src/net)
include_directories(src/base)
include_directories(3rdparty/cjson)
include_directories(3rdparty/spdlog/include)
include_directories(src)

add_subdirectory(3rdparty)
add_subdirectory(src)
add_subdirectory(service)
#add_subdirectory(test)
add_subdirectory(example)
#add_subdirectory(demo_game)


#add_definitions(-std=c++2a)


