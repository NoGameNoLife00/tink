cmake_minimum_required(VERSION 3.1)

include(cmake/utils.cmake)
include(cmake/messagecolor.cmake)
tink_extract_version()
project(tink VERSION ${TINK_VERSION} LANGUAGES CXX C)

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose Release or Debug" FORCE)
ENDIF()

message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})

message( "** INFO ------------------------------------------------")
message( "** STATUS Build tink ${TINK_VERSION}")


# 设置可执行文件输出目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(ENABLE_LUA ON)

message(${ENABLE_LUA})
#if(ENABLE_LUA)
#    set(LUA_ROOT_DIR "${PROJECT_SOURCE_DIR}/3rdparty/lua")
#    option(WITH_LUA "enable lua of tink" ON)
#
#    set(LUA_LIBRARY ${LUA_ROOT_DIR}/lib/liblua.a)
#
#    add_custom_target(build_lua
#            COMMAND ${CMAKE_MAKE_PROGRAM}
#            WORKING_DIRECTORY ${LUA_ROOT_DIR}
#            COMMENT "Original lua makefile target")
##    add_library(lua_SHARED SHARED IMPORTED)
#    add_library(lua STATIC IMPORTED)
#    set_property(TARGET lua APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
#    set_target_properties(lua PROPERTIES IMPORTED_LOCATION_NOCONFIG "${LUA_LIBRARY}")
##    add_dependencies(lua_SHARED build_lua)
#    add_dependencies(lua build_lua)
#    list(APPEND SYS_LIBS lua)
#    message("lua-path:${LUA_ROOT_DIR}")
#endif()


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
        set(JEMALLOC_INCLUDE_DIR ${JEMALLOC_ROOT_DIR}/include)
        set(JEMALLOC_LIBRARIES ${JEMALLOC_ROOT_DIR}/lib)
        add_definitions(-DJEMALLOC_NO_DEMANGLE)
        include_directories(${JEMALLOC_INCLUDE_DIR})
    endif()

    list(APPEND SYS_LIBS "dl" "pthread" "rt")
    # thirdparty code
    # do not set unnecessary flags before thirdparty dirs
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -g -fno-omit-frame-pointer")
endif()

list(APPEND SYS_LIBS ${SPDLOG_LIBS})


include_directories(3rdparty/cjson)
include_directories(3rdparty/spdlog/include)
include_directories(3rdparty/lua)
include_directories(src)
include_directories(service)


add_subdirectory(3rdparty)
add_subdirectory(src)
add_subdirectory(service)
#add_subdirectory(test)
#add_subdirectory(example)
#add_subdirectory(demo_game)
