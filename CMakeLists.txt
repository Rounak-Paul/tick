cmake_minimum_required(VERSION 3.16)
project(tick
  VERSION 0.1.0
  DESCRIPTION "Temporal Deterministic Language (TDL) Compiler"
  LANGUAGES CXX
)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Compiler Sources
set(COMPILER_SOURCES
  src/compiler/lexer.cpp
  src/compiler/parser.cpp
  src/compiler/ast.cpp
)

set(COMPILER_HEADERS
  src/compiler/lexer.h
  src/compiler/parser.h
  src/compiler/ast.h
  src/compiler/token.h
)

# Runtime Sources
set(RUNTIME_SOURCES
  src/runtime/channel.cpp
  src/runtime/clock.cpp
  src/runtime/process.cpp
  src/runtime/scheduler.cpp
)

set(RUNTIME_HEADERS
  src/runtime/channel.h
  src/runtime/clock.h
  src/runtime/process.h
  src/runtime/scheduler.h
  src/runtime/runtime.h
)

# Code Generator Sources
set(CODEGEN_SOURCES
  src/codegen/codegen.cpp
  src/codegen/codegen_new.cpp
)

set(CODEGEN_HEADERS
  src/codegen/codegen.h
  src/codegen/codegen_new.h
)

# Build the main executable
add_executable(tdl
  src/main.cpp
  ${COMPILER_SOURCES}
  ${COMPILER_HEADERS}
  ${RUNTIME_SOURCES}
  ${RUNTIME_HEADERS}
  ${CODEGEN_SOURCES}
  ${CODEGEN_HEADERS}
)

# Enable testing
enable_testing()

# Add test executable (optional, to be implemented)
# add_subdirectory(tests)

# Install targets
install(TARGETS tdl DESTINATION bin)

# Print configuration summary
message(STATUS "=== TDL Project Configuration ===")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Output Directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "==================================")
