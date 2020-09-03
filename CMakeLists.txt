cmake_minimum_required(VERSION 3.1)

include(cmake/utils.cmake)
include(cmake/messagecolor.cmake)
tink_extract_version()
project(tink VERSION ${TINK_VERSION} LANGUAGES CXX C)

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose Release or Debug" FORCE)
ENDIF()

MESSAGE(STATUS "Build type: " ${CMAKE_BUILD_TYPE})

MESSAGE( ** INFO "------------------------------------------------")
MESSAGE( ** STATUS "Build tink ${TINK_VERSION}")


#INCLUDE(cmake/load_3rdparty.cmake)
include_directories(src/interface)
include_directories(src/net)
include_directories(src/base)
include_directories(3rdparty/cjson)
include_directories(3rdparty/easyloggingpp/src)
include_directories(3rdparty/spdlog/include)
include_directories(src)

add_subdirectory(3rdparty)
add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(example)
add_subdirectory(demo_game)
#include(ExternalProject)
#ExternalProject_Add(libhello
#        URL    "sharelib.zip"
#        DOWNLOAD_DIR ${libhello_download}
#        SOURCE_DIR        "${libhello_src}"
#        CONFIGURE_COMMAND ${CMAKE_COMMAND} ${libhello_src}
#        -DCMAKE_INSTALL_PREFIX=${libhello_install}
#        BINARY_DIR  ${libhello_binary}
#        INSTALL_COMMAND ${CMAKE_MAKE_PROGRAM} install
#        )

#set(CMAKE_CXX_STANDARD 14)
#include_directories(D:\\DEV\\cygwin\\usr\\include)
#link_directories(D:\\DEV\\cygwin\\lib)





add_definitions(-std=c++2a)

#file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true *.h *.cpp)


