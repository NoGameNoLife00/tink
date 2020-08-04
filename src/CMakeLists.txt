
#设置 BINARY 为项目名IndexProject
set(BINARY ${CMAKE_PROJECT_NAME})

# 1
# add_executable(ExampleProject main.cpp file1.cpp file1.h)
# 2
include_directories(../libs/cjson)
include_directories(../libs/easyloggingpp)
include_directories(interface)
include_directories(net)
include_directories(utils)

file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true *.h *.cpp)
set(SOURCES  ../libs/cjson/cJSON.c ../libs/easyloggingpp/easylogging++.cc ${SOURCES})
add_definitions(-D_ELPP_THREAD_SAFE)
#add_executable(${BINARY}_run ${SOURCES})
# 将框架代码作为静态链接库使用
add_library(${BINARY}_lib STATIC ${SOURCES})
target_link_libraries(${BINARY}_lib -pthread)
#target_link_libraries(${BINARY}_lib Threads::Threads)