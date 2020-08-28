set(BINARY ${CMAKE_PROJECT_NAME})

include(FindProtobuf)
find_package(Protobuf REQUIRED)
if(PROTOBUF_FOUND)
    message(STATUS "protobuf library found")
else()
    message(FATAL_ERROR "protobuf library is needed but cant be found")
endif()

set(PROTOBUF_NAME "pbtest")
set(PROTOBUF_FILE ${PROTOBUF_NAME}.proto)
set(PROTO_SRCS ${PROTOBUF_NAME}.pb.cc)
set(PROTO_HDRS ${PROTOBUF_NAME}.pb.h)

include_directories(${PROTOBUF_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
#protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOBUF_FILE})

MESSAGE("** gen protobuf file...")

add_custom_command(OUTPUT ${PROTO_SRCS} ${PROTO_HDRS}
        COMMAND protoc
        ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/${PROTOBUF_FILE} -I${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND cp
        ARGS -f ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_HDRS} ${CMAKE_CURRENT_SOURCE_DIR}/
        COMMAND cp
        ARGS -f ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/
        DEPENDS ${PROTOBUF_FILE})

#file(COPY ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_HDRS} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/)
#file(COPY ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_SRCS} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/)

set_source_files_properties(${PROTO_SRCS} PROPERTIES COMPILE_FLAGS "-Wno-conversion -Wno-shadow")
include_directories(${PROJECT_BINARY_DIR})
add_library(${BINARY}_proto STATIC ${PROTO_SRCS} ${PROTO_HDRS})
#add_library(${BINARY}_proto STATIC ${PROTOBUF_FILE}.pb.cc)
target_link_libraries(${BINARY}_proto ${PROTOBUF_LIBRARY} pthread)

