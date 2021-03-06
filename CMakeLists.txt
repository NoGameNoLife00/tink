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

if(CMAKE_COMPILER_IS_GNUCC)
    option(ENABLE_JEMALLOC "enable jemalloc" ON)
    if(ENABLE_JEMALLOC)
        set(JEMALLOC_ROOT_DIR "${PROJECT_SOURCE_DIR}/3rdparty/jemalloc")
        option(WITH_JEMALLOC "enable jemalloc of tink" ON)

        add_custom_command(OUTPUT "${JEMALLOC_ROOT_DIR}/include/jemalloc/jemalloc.h"
                COMMAND ./autogen.sh
                WORKING_DIRECTORY ${JEMALLOC_ROOT_DIR})
        add_custom_command(OUTPUT "${JEMALLOC_ROOT_DIR}/lib/libjemalloc.a" "${JEMALLOC_ROOT_DIR}/lib/libjemalloc.so"
                COMMAND ${CMAKE_MAKE_PROGRAM}
                DEPENDS "${JEMALLOC_ROOT_DIR}/include/jemalloc/jemalloc.h"
                WORKING_DIRECTORY ${JEMALLOC_ROOT_DIR}
                COMMENT "Making external library jemalloc")
        add_custom_target(build_jemalloc
                DEPENDS "${JEMALLOC_ROOT_DIR}/lib/libjemalloc.a")

        add_library(jemalloc_SHARED SHARED IMPORTED)
        add_library(jemalloc STATIC IMPORTED)
        set_target_properties(jemalloc PROPERTIES IMPORTED_LOCATION "${JEMALLOC_ROOT_DIR}/lib/libjemalloc.a")
        set_target_properties(jemalloc_SHARED PROPERTIES IMPORTED_LOCATION "${JEMALLOC_ROOT_DIR}/lib/libjemalloc.so")

#        MESSAGE("libjemalloc-dir: ${JEMALLOC_ROOT_DIR}/lib/libjemalloc.so ${JEMALLOC_ROOT_DIR}/lib/libjemalloc.a")
        list(APPEND SYS_LIBS jemalloc)
        add_dependencies(jemalloc build_jemalloc)
        add_dependencies(jemalloc_SHARED build_jemalloc)
    endif()

    list(APPEND SYS_LIBS "dl" "pthread" "rt")
    # thirdparty code
    # do not set unnecessary flags before thirdparty dirs
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -g -fno-omit-frame-pointer -ljemalloc")
endif()

list(APPEND SYS_LIBS ${SPDLOG_LIBS})


#include_directories(src/net)
#include_directories(src/base)
include_directories(3rdparty/cjson)
include_directories(3rdparty/spdlog/include)
include_directories(src)
include_directories(service)

add_subdirectory(3rdparty)
add_subdirectory(src)
add_subdirectory(service)
#add_subdirectory(test)
#add_subdirectory(example)
#add_subdirectory(demo_game)
