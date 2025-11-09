cmake_minimum_required(VERSION 3.10)
project(tick)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3 -pthread")

file(GLOB_RECURSE SOURCES
    src/main.cpp
    src/compiler/*.cpp
    src/runtime/*.cpp
)

add_executable(tick ${SOURCES})

target_link_libraries(tick pthread)

add_subdirectory(tests)

install(TARGETS tick DESTINATION bin)
