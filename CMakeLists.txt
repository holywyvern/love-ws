cmake_minimum_required (VERSION 3.0)

project (lua-ws)

if(NOT MSVC)
    add_compile_options(-std=c++11 -Wall -Wextra -Wsign-conversion)
else()
    add_compile_options(/W1)
endif()

add_library(ws MODULE
  src/ws.cpp
)

target_include_directories(ws
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/include
)

if(APPLE)
    set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
endif()

find_package(OpenSSL REQUIRED)
find_package(Lua REQUIRED)

target_include_directories(ws PUBLIC ${OPENSSL_INCLUDE_DIR})
target_include_directories(ws PUBLIC ${LUA_INCLUDE_DIR})

target_link_libraries(ws PRIVATE ${OPENSSL_LIBRARIES})
target_link_libraries(ws PRIVATE ${LUA_LIBRARIES})


if(WIN32)
    target_link_libraries(ws PRIVATE ws2_32 wsock32)
endif()