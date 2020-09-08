#project(demo_game)

set(BINARY demo_game)

add_subdirectory(protobuf)

include_directories(logic)
include_directories(protobuf)

file(GLOB_RECURSE APP_SOURCES LIST_DIRECTORIES true *.h *.cpp)

set(SOURCES ${APP_SOURCES})

add_executable(${BINARY} ${APP_SOURCES})

SET(EXECUTABLE_OUTPUT_PATH  ./bin)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/etc DESTINATION ./)
#file(COPY ${PROJECT_SOURCE_DIR}/etc/log.conf DESTINATION ./)
target_link_libraries(${BINARY} PUBLIC ${CMAKE_PROJECT_NAME}_lib demo_proto)
set_property(TARGET ${BINARY}  PROPERTY CXX_STANDARD 17)