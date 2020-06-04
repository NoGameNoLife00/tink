cmake_minimum_required(VERSION 3.0)
project(tink)

set(CMAKE_CXX_STANDARD 14)
include_directories(D:\\DEV\\cygwin\\usr\\include)
link_directories(D:\\DEV\\cygwin\\lib)


include_directories(src/interface)
include_directories(src/net)
include_directories(src/utils)
include_directories(libs/cjson)
include_directories(src)

add_subdirectory(src)
add_subdirectory(libs/googletest)
add_subdirectory(test)
add_subdirectory(example)
add_subdirectory(app)
#file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true *.h *.cpp)


