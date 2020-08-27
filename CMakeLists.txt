cmake_minimum_required(VERSION 3.1)
project(tink)
#include(ExternalProject)
#
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
include_directories(src/interface)
include_directories(src/net)
include_directories(src/base)
include_directories(libs/cjson)
include_directories(libs/easyloggingpp)
include_directories(src)
include_directories(protobuf)


add_subdirectory(src)
add_subdirectory(libs/googletest)
add_subdirectory(test)
add_subdirectory(example)
add_subdirectory(app)
add_subdirectory(protobuf)
#file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true *.h *.cpp)


