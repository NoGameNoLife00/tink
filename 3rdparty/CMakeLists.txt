cmake_minimum_required(VERSION 3.1)

PROJECT(${CMAKE_PROJECT_NAME}_3rdparty)

add_subdirectory(googletest)
add_subdirectory(spdlog)
add_subdirectory(libcoroutine)
message(${ENABLE_LUA})
if(ENABLE_LUA)
    set(LUA_ROOT_DIR "${PROJECT_SOURCE_DIR}/lua")
    option(WITH_LUA "enable lua of tink" ON)

    aux_source_directory(${LUA_ROOT_DIR} LUA_SRC)

    LIST(REMOVE_ITEM LUA_SRC lua.c luac.c)

    add_library(lua STATIC ${LUA_SRC})
    message("lua-path:${LUA_ROOT_DIR}")
endif()