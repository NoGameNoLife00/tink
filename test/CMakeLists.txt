set(BINARY ${CMAKE_PROJECT_NAME}_tst)

file(GLOB_RECURSE SRC_SOURCES  ${PROJECT_SOURCE_DIR}/demo_game/logic/*.cpp ${PROJECT_SOURCE_DIR}/demo_game/logic/*.h)
file(GLOB_RECURSE TEST_SOURCES LIST_DIRECTORIES false *.h *.cpp)
set(SOURCES ${TEST_SOURCES} ${SRC_SOURCES})
add_executable(${BINARY} ${SOURCES})
add_test(NAME ${BINARY} COMMAND ${BINARY})
# 链接src生成的lib库和gtest库
target_link_libraries(${BINARY} PUBLIC ${CMAKE_PROJECT_NAME}_lib gtest)