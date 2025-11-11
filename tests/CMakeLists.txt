cmake_minimum_required(VERSION 3.10)

file(GLOB COMPILER_SOURCES
    ../src/compiler/*.cpp
)

file(GLOB RUNTIME_SOURCES
    ../src/runtime/*.cpp
)

add_executable(test_all
    test_all.cpp
    ${COMPILER_SOURCES}
    ${RUNTIME_SOURCES}
)
target_link_libraries(test_all pthread)

add_executable(benchmark
    benchmark.cpp
    ${COMPILER_SOURCES}
    ${RUNTIME_SOURCES}
)
target_link_libraries(benchmark pthread)

enable_testing()
add_test(NAME AllTests COMMAND test_all)
