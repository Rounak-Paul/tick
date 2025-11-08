# TDL: Temporal Deterministic Language

A programming language with **automatic parallelization** that guarantees **deterministic, race-free execution**.

## Quick Start

```bash
# Build
cd /Users/duke/Code/tick
cmake .
make

# Run an example
./build/bin/tdl examples/fibonacci.tdl  # Output: 55
```

## What is TDL?

Traditional concurrent programming is fundamentally broken:

| Issue | Traditional | TDL |
|-------|-----------|-----|
| Race conditions | ✗ Unavoidable | ✓ Impossible |
| Non-determinism | ✗ Every run different | ✓ Identical output |
| Synchronization | ✗ Locks, mutexes | ✓ Automatic |
| Deadlocks | ✗ Possible | ✓ Impossible |
| Debugging | ✗ Nightmare | ✓ Reproducible |

**TDL solves this by making parallelism deterministic by design.**

## How It Works

```tdl
func main() {
  let a: int = compute1();   // Parallel
  let b: int = compute2();   // Parallel (independent of a)
  let c: int = a + b;        // Waits for a and b
  println(c);                // Deterministic output
}
```

The compiler:
1. Analyzes which statements depend on which variables
2. Groups independent statements into execution layers
3. Runs each layer in parallel on multiple threads
4. Maintains deterministic ordering between layers

**Result:** Every execution produces identical output with automatic parallelization.

## Features

✅ **Automatic parallelization** - Independent statements run in parallel  
✅ **Deterministic execution** - Identical output every time  
✅ **No race conditions** - By design, not luck  
✅ **No locks/mutexes** - Built-in synchronization  
✅ **No deadlocks** - Impossible by architecture  
✅ **C-style syntax** - Familiar to programmers  
✅ **Functions with recursion** - General-purpose computing  
✅ **Static variables** - Deterministic state management  

## Language Basics

### Functions

```tdl
func fibonacci(int n) -> int {
  if (n <= 1) {
    return n;
  }
  return fibonacci(n - 1) + fibonacci(n - 2);
}

func main() {
  println(fibonacci(10));  // Output: 55
}
```

### Variables and Control Flow

```tdl
func main() {
  let x: int = 10;
  
  if (x > 5) {
    println(x);
  }
  
  let i: int = 0;
  while (i < 3) {
    println(i);
    i = i + 1;
  }
}
```

### Automatic Parallelization

```tdl
func main() {
  let a: int = 100 + 200;     // Layer 1 (parallel)
  let b: int = 300 + 400;     // Layer 1 (parallel)
  let c: int = 500 + 600;     // Layer 1 (parallel)
  
  let sum: int = a + b + c;   // Layer 2 (waits for Layer 1)
  println(sum);               // Output: 2100
}
```

## Examples

Run any of these:

```bash
./build/bin/tdl examples/fibonacci.tdl                    # Recursive functions
./build/bin/tdl examples/simple.tdl                       # Basic I/O
./build/bin/tdl examples/counter.tdl                      # Static variables
./build/bin/tdl examples/deterministic_accumulator.tdl    # State management
./build/bin/tdl examples/test_parallel_analysis.tdl       # Parallelization
```

All produce **identical output on every run** ✅

## Documentation

Complete documentation is available in `/docs`:

- **[Getting Started](./docs/getting_started.md)** - Write your first program
- **[Language Reference](./docs/language_reference.md)** - Complete syntax
- **[Parallelism Guide](./docs/parallelism_guide.md)** - How automatic parallelization works
- **[API Reference](./docs/api_reference.md)** - Built-in functions

## Build

### Requirements

- C++17 compiler (g++, clang, or MSVC)
- CMake 3.16+
- macOS, Linux, or Windows

### Build Instructions

```bash
cd /Users/duke/Code/tick
cmake .
make

# Executable is at ./build/bin/tdl
```

## Project Structure

```
src/
├── compiler/
│   ├── lexer.cpp/h          - Tokenization
│   ├── parser.cpp/h         - AST construction
│   ├── token.h              - Token definitions
│   └── ast.cpp/h            - AST nodes
├── runtime/
│   ├── executor.cpp/h            - Execution engine
│   ├── dependency_analyzer.cpp/h - Parallel dependency analysis
│   └── scheduler.cpp/h           - Program orchestration
└── main.cpp                 - CLI entry point

examples/                    - Example TDL programs
docs/                        - Documentation
test/                        - Test suite
```

## Architecture

**Compilation Pipeline:**

```
TDL Source (.tdl)
    ↓ [Lexer]
Tokens
    ↓ [Parser]
Abstract Syntax Tree (AST)
    ↓ [Executor]
Dependency Analysis
    ↓ [Parallelization]
Parallel Execution Layers
    ↓ [Thread Pool]
Deterministic Output
```

## The Problem TDL Solves

Every major programming language struggles with concurrent programming:

**Java, C++, Go, Rust:** All have race conditions, deadlocks, and non-deterministic bugs that are nearly impossible to debug.

**TDL's approach:** Make parallelism deterministic by design. Independent operations automatically run in parallel, dependent operations wait synchronously. No locks, no synchronization primitives, no way to create race conditions.

## Determinism Guarantee

Every TDL program produces identical results because:

1. **No race conditions** - Parallelization is safe by design
2. **Deterministic ordering** - Independent layers execute in consistent order
3. **No randomness** - All operations are deterministic
4. **Static variables** - Always initialize and behave identically

This makes TDL ideal for:
- Scientific computation (reproducible results)
- Financial systems (auditable calculations)
- Data processing pipelines (consistent transformations)
- Simulation (repeatable experiments)
- Systems that must be debuggable

## Key Insight

**Every execution produces identical results with zero race conditions, and the language automatically manages all parallelism.**

This is like VHDL/Verilog (hardware description languages) but for software running on CPUs.

## Version

**v1.0.0 Beta** - Core language complete and working

- ✅ Lexer and parser complete
- ✅ Automatic parallelization via dependency analysis
- ✅ Deterministic execution proven
- ✅ All examples working
- ✅ Professional documentation

## Next Steps

1. Read [Getting Started](./docs/getting_started.md)
2. Try the examples
3. Build your first program
4. Join the community

## License

See LICENSE file for details.

---

**TDL: Deterministic parallelism is not a hope, but a guarantee.**
