cmake_minimum_required (VERSION 3.0)

project (lua-ws)

if(NOT MSVC)
    add_compile_options(-std=c++11 -Wall -Wextra -Wsign-conversion)
else()
    add_compile_options(/W1)
endif()

add_library(ws MODULE
  src/lua_ws_client.cpp 
  src/lua_ws_server.cpp
  src/lua_ws_server_channel.cpp
  src/lua_ws.cpp
)

target_include_directories(ws
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/include
)

if(APPLE)
    set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
endif()

add_definitions( -DBOOST_ALL_NO_LIB )
set(Boost_USE_STATIC_LIBS        ON) # only find static libs
set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)

find_package(OpenSSL REQUIRED)
find_package(Lua REQUIRED)
find_package(Boost 1.54.0 COMPONENTS system thread filesystem REQUIRED)

target_include_directories(ws PUBLIC ${OPENSSL_INCLUDE_DIR})
target_include_directories(ws PUBLIC ${LUA_INCLUDE_DIR})
target_include_directories(ws PRIVATE ${Boost_INCLUDE_DIR})
target_include_directories(ws PRIVATE ${PROJECT_SOURCE_DIR}/vendor/simple-websocket-server)

target_link_libraries(ws PRIVATE ${OPENSSL_LIBRARIES})
target_link_libraries(ws PRIVATE ${LUA_LIBRARIES})
target_link_libraries(ws PRIVATE ${Boost_LIBRARIES})


if(WIN32)
    target_link_libraries(ws PRIVATE ws2_32 wsock32)
endif()