set(BINARY ${CMAKE_PROJECT_NAME})

include(FindProtobuf)
find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS pbtest.proto)

file(COPY ${PROTO_HDRS} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/)
file(COPY ${PROTO_SRCS} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/)

add_library(${BINARY}_proto STATIC ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(${BINARY}_proto ${PROTOBUF_LIBRARY} pthread)

