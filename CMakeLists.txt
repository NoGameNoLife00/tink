cmake_minimum_required(VERSION 3.1)

include(cmake/utils.cmake)
tink_extract_version()
project(tink VERSION ${TINK_VERSION} LANGUAGES CXX C)


MESSAGE(** INFO "------------------------------------------------")
MESSAGE(** STATUS "Build tink ${TINK_VERSION}")

#INCLUDE(cmake/load_3rdparty.cmake)

add_subdirectory(3rdparty)
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

include_directories(src/interface)
include_directories(src/net)
include_directories(src/base)
include_directories(3rdparty/cjson)
include_directories(3rdparty/easyloggingpp/src)
include_directories(src)
include_directories(protobuf)


add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(example)
add_subdirectory(app)
add_subdirectory(protobuf)
#file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true *.h *.cpp)


