cmake_minimum_required(VERSION 3.16)
project(KVolt)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Main executable
add_executable(kvolt
    src/main.cpp
    src/cache/lru_cache.cpp
    src/cache/ttl_manager.cpp
)

target_include_directories(kvolt PRIVATE src)