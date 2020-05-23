cmake_minimum_required(VERSION 2.8.4)
project(tink)

set(CMAKE_CXX_STANDARD 14)
include_directories(D:\\DEV\\cygwin\\usr\\include)
link_directories(D:\\DEV\\cygwin\\lib)
include_directories(src/interface)
include_directories(src/net)
include_directories(src)
include_directories(app)
add_executable(tink app/main.cpp
        src/interface/interface_server.h
        src/net/server.h src/net/server.cpp
        src/interface/interface_conn.h
        src/error_code.h src/net/connection.cpp src/net/connection.h)
add_executable(example_client example/client.cpp)