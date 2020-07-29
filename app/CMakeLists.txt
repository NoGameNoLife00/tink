set(BINARY ${CMAKE_PROJECT_NAME}_app)

file(GLOB_RECURSE APP_SOURCES LIST_DIRECTORIES false *.h *.cpp)

set(SOURCES ${APP_SOURCES})

#add_test(NAME ${BINARY} COMMAND ${BINARY})
add_executable(${BINARY} ${APP_SOURCES})

target_link_libraries(${BINARY} PUBLIC ${CMAKE_PROJECT_NAME}_lib)


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
#add_executable(tink_app app/main.cpp)
#add_executable(tink_app main.cpp
#        ../libs/cjson/cJSON.h
#        ../libs/cjson/cJSON.c
#        ../src/interface/iserver.h
#        ../src/net/server.h
#        ../src/net/server.cpp
#        ../src/interface/iconnection.h
#        ../src/error_code.h
#        ../src/net/connection.cpp
#        ../src/net/connection.h
#        ../src/net/request.cpp
#        ../src/net/request.h
#        ../src/net/base_router.cpp
#        ../src/net/base_router.h
#        ../src/utils/global_mng.cpp
#        ../src/utils/global_mng.h
#        ../src/error_code.h
#        ../src/net/connection.cpp
#        ../src/net/connection.h
#        ../src/net/request.cpp
#        ../src/net/request.h
#        ../src/net/base_router.cpp
#        ../src/net/base_router.h
#        ../src/type.h
#        ../src/interface/singleton.h
#        ../src/net/message.cpp
#        ../src/net/message.h
#        ../src/net/datapack.cpp
#        ../src/net/datapack.h
#        ../src/interface/irequest.h
#        ../src/interface/irouter.h
#        ../src/interface/singleton.h
#        ../src/interface/imessage.h
#        ../src/interface/idatapack.h)