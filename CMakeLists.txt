cmake_minimum_required(VERSION 3.0)
project(tink)

set(CMAKE_CXX_STANDARD 14)
include_directories(D:\\DEV\\cygwin\\usr\\include)
link_directories(D:\\DEV\\cygwin\\lib)


include_directories(src/interface)
include_directories(src/net)
include_directories(src/utils)
include_directories(libs/cjson)
include_directories(src)
include_directories(app)

add_subdirectory(src)
add_subdirectory(libs/googletest)
add_subdirectory(test)
#file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true *.h *.cpp)
add_executable(tink app/main.cpp
        libs/cjson/cJSON.h
        libs/cjson/cJSON.c
        src/interface/iserver.h
        src/net/server.h
        src/net/server.cpp
        src/interface/iconnection.h
        src/error_code.h
        src/net/connection.cpp
        src/net/connection.h
        src/net/request.cpp
        src/net/request.h
        src/net/base_router.cpp
        src/net/base_router.h
        src/utils/global_mng.cpp
        src/utils/global_mng.h
        src/error_code.h
        src/net/connection.cpp
        src/net/connection.h
        src/net/request.cpp
        src/net/request.h
        src/net/base_router.cpp
        src/net/base_router.h
        src/type.h
        src/interface/singleton.h
        src/net/message.cpp
        src/net/message.h
        src/net/datapack.cpp
        src/net/datapack.h)
add_executable(example_client example/client.cpp src/interface/irequest.h src/interface/irouter.h src/interface/singleton.h src/interface/imessage.h src/interface/idatapack.h)
