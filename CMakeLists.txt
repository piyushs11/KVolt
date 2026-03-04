cmake_minimum_required(VERSION 3.16)
project(KVolt)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(kvolt
    src/main.cpp
    src/cache/lru_cache.cpp
    src/cache/ttl_manager.cpp
    src/server/tcp_server.cpp
)

target_include_directories(kvolt PRIVATE src)

# Link Winsock only on Windows
if(WIN32)
    target_link_libraries(kvolt ws2_32)
endif()