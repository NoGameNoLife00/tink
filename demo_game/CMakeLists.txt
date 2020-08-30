project(demo_game)

set(BINARY demo_game)

include_directories(logic)
include_directories(protobuf)

add_subdirectory(protobuf)

file(GLOB_RECURSE APP_SOURCES LIST_DIRECTORIES false *.h *.cpp)

set(SOURCES ${APP_SOURCES})

add_executable(${BINARY} ${APP_SOURCES})

SET(EXECUTABLE_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/bin)
target_link_libraries(${BINARY} PUBLIC ${CMAKE_PROJECT_NAME}_lib demo_proto)