cmake_minimum_required(VERSION 3.10)
project(tick)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2")

file(GLOB_RECURSE SOURCES
    src/main.cpp
    src/compiler/*.cpp
    src/runtime/compiler.cpp
)

add_executable(tick ${SOURCES})

install(TARGETS tick DESTINATION bin)
