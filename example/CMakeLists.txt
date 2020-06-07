set(BINARY ${CMAKE_PROJECT_NAME}_example)


add_executable(example_client client.cpp )

target_link_libraries(example_client PUBLIC ${CMAKE_PROJECT_NAME}_lib)