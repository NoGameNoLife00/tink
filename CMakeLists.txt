cmake_minimum_required(VERSION 2.8.4)
project(tink)

set(CMAKE_CXX_STANDARD 14)
include_directories(D:\\DEV\\cygwin\\usr\\include)
link_directories(D:\\DEV\\cygwin\\lib)
include_directories(src/interface)
include_directories(src/net)
include_directories(src/libs)
include_directories(src)
include_directories(app)
add_executable(tink app/main.cpp
        libs/cJSON.h
        libs/cJSON.c
        src/interface/iserver.h
        src/net/server.h src/net/server.cpp
        src/interface/iconnection.h
        src/error_code.h
        src/net/connection.cpp
        src/net/connection.h
        src/net/request.cpp
        src/net/request.h
        src/net/base_router.cpp
        src/net/base_router.h)
add_executable(example_client example/client.cpp src/interface/irequest.h src/interface/irouter.h src/utils/global.cpp src/utils/global.h)        src/error_code.h src/net/connection.cpp src/net/connection.h src/net/request.cpp src/net/request.h src/net/base_router.cpp src/net/base_router.h src/type.h)
add_executable(example_client example/client.cpp src/interface/irequest.h src/interface/irouter.h)