
#设置 BINARY 为项目名IndexProject
set(BINARY ${CMAKE_PROJECT_NAME})

include_directories(base)
include_directories(net)

file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true *.h *.cpp)
set(SOURCES ${SOURCES} ../3rdparty/cjson/cJSON.c)
add_definitions(-DELPP_THREAD_SAFE
        -DELPP_STL_LOGGING
        -DELPP_LOG_UNORDERED_MAP
        -DELPP_LOG_STD_ARRAY
        -DELPP_LOG_UNORDERED_SET)

#add_definitions(-std=c++2a)

# 将框架代码作为静态链接库使用
add_library(${BINARY}_lib STATIC ${SOURCES})
target_link_libraries(${BINARY}_lib ${SPDLOG_LIBS} -pthread)
set_property(TARGET ${BINARY}_lib  PROPERTY CXX_STANDARD 20)