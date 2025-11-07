# TDL - Temporal Deterministic Language

A compiler and runtime for a **Temporal Deterministic Language** - a domain-specific language designed for writing deterministic, time-synchronized concurrent programs. TDL makes it easy and intuitive to express temporal behaviors, synchronous data processing pipelines, and clock-driven processes.

## Features

- **Clock-based synchronization**: Define clocks with specific frequencies (Hz) that drive process execution
- **Deterministic execution**: All operations are synchronized to clock ticks - no race conditions
- **Channel-based communication**: Type-safe, bounded channels for inter-process communication
- **Parallel processes**: Express concurrent processes naturally with the `par` block
- **Python-like syntax**: Familiar, intuitive syntax similar to Python and C++
- **Compile to C++**: Generates optimized C++ code that can be further compiled with standard tools

## Quick Start

### Installation

```bash
cd /Users/duke/Code/tick
mkdir -p build
cd build
cmake ..
make
```

The compiled `tdl` executable will be in `build/bin/`

### Running a TDL Program

```bash
./bin/tdl examples/dsp.tdl
```

With options:
```bash
./bin/tdl examples/dsp.tdl --compile-only --output generated.cpp
```

## Language Syntax

### Clocks

Define a clock with a specific frequency:

```tdl
clock sys = 1000hz;  // 1000 Hz clock
```

### Processes

Define processes that execute code:

```tdl
proc myProcess(chan<int> in, chan<int> out) {
  on sys.tick {
    // Code runs on each clock tick
  }
}
```

### Channels

Create bounded, typed channels for communication between processes:

```tdl
chan<int> myChannel(bound=4);  // Channel with 4-element buffer
```

### Sending and Receiving

```tdl
myChannel.send(42);           // Send a value

if (myChannel.try_recv() is some value) {
  println(value);             // Pattern matching on received values
}
```

### Parallel Execution

Run multiple processes in parallel:

```tdl
par {
  producer(channelA);
  consumer(channelA);
}
```

### Variables

Variables with optional `static` keyword (persistent across ticks):

```tdl
let x: int = 42;           // Local variable
static counter: int = 0;   // Persists across ticks
```

## Project Structure

```
tick/
├── src/
│   ├── compiler/          # Lexer, Parser, AST
│   │   ├── lexer.h/cpp    # Tokenization
│   │   ├── parser.h/cpp   # Syntax analysis
│   │   ├── token.h        # Token definitions
│   │   └── ast.h/cpp      # Abstract Syntax Tree
│   ├── runtime/           # Runtime execution engine
│   │   ├── clock.h/cpp    # Clock management
│   │   ├── channel.h      # Channel implementation
│   │   ├── process.h/cpp  # Process abstraction
│   │   ├── scheduler.h/cpp # Process scheduler
│   │   └── runtime.h      # Runtime API
│   ├── codegen/           # Code generation
│   │   ├── codegen.h/cpp  # C++ code generator
│   └── main.cpp           # CLI entry point
├── examples/              # Example TDL programs
│   └── dsp.tdl           # Digital Signal Processing example
├── tests/                 # Test suite
├── docs/                  # Documentation
├── CMakeLists.txt        # Build configuration
└── README.md             # This file
```

## Example: Digital Signal Processing Pipeline

See `examples/dsp.tdl` for a complete example that demonstrates:
- Clock-driven synchronization
- Producer-consumer pattern
- Channel-based communication
- Pattern matching with `is some`
- Static variables for state

## Build and Development

### Prerequisites

- C++17 or later
- CMake 3.16+
- macOS, Linux, or Windows with a C++ compiler

### Building

```bash
cd /Users/duke/Code/tick
mkdir -p build
cd build
cmake ..
make -j4
```

### Running Compiler

```bash
./bin/tdl <tdl-file> [options]
```

Options:
- `--compile-only`: Only generate C++ without execution details
- `--output FILE`: Write generated C++ to a file
- `--help`: Show help message
- `--version`: Show version

## Architecture

### Compilation Pipeline

1. **Lexer** (`src/compiler/lexer.cpp`) - Tokenizes source code
2. **Parser** (`src/compiler/parser.cpp`) - Builds Abstract Syntax Tree (AST)
3. **Code Generator** (`src/codegen/codegen.cpp`) - Converts AST to C++ code

### Runtime

- **Clock** - Manages time and triggers ticks
- **Channel** - Thread-safe, bounded message queues
- **Process** - Encapsulates executable code
- **Scheduler** - Manages process execution

## Future Enhancements

- [ ] LLVM IR backend for direct machine code generation
- [ ] Full type system with custom types and structs
- [ ] Module system for code reuse
- [ ] Optimization passes
- [ ] Formal verification support
- [ ] Debugging and tracing support
- [ ] Better error messages with source location info
- [ ] Comprehensive standard library

## Contributing

This is an early-stage project. Contributions are welcome!

## License

See LICENSE file.
