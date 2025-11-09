cmake_minimum_required(VERSION 3.10)

file(GLOB COMPILER_SOURCES
    ../src/compiler/*.cpp
)

file(GLOB RUNTIME_SOURCES
    ../src/runtime/*.cpp
)

add_executable(test_core test_core.cpp)
target_link_libraries(test_core pthread)

add_executable(test_compiler 
    test_compiler.cpp
    ${COMPILER_SOURCES}
)
target_link_libraries(test_compiler pthread)

add_executable(test_runtime
    test_runtime.cpp
    ${COMPILER_SOURCES}
    ${RUNTIME_SOURCES}
)
target_link_libraries(test_runtime pthread)

add_executable(test_features
    test_features.cpp
    ${COMPILER_SOURCES}
    ${RUNTIME_SOURCES}
)
target_link_libraries(test_features pthread)

add_executable(benchmark
    benchmark.cpp
    ${COMPILER_SOURCES}
    ${RUNTIME_SOURCES}
)
target_link_libraries(benchmark pthread)

enable_testing()
add_test(NAME CoreTests COMMAND test_core)
add_test(NAME CompilerTests COMMAND test_compiler)
add_test(NAME RuntimeTests COMMAND test_runtime)
add_test(NAME FeatureTests COMMAND test_features)
