#设置 BINARY 为项目名IndexProject
set(BINARY ${CMAKE_PROJECT_NAME})

include_directories(base)
include_directories(net)


file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true *.h *.cpp)
set(SOURCES ${SOURCES} ../3rdparty/cjson/cJSON.c)

#add_definitions(-std=c++2a)
#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../etc DESTINATION ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/../)

add_executable(${BINARY} ${SOURCES})
target_link_libraries(${BINARY} ${SPDLOG_LIBS} -pthread -ldl libcoroutine)
set_property(TARGET ${BINARY}  PROPERTY CXX_STANDARD 20)