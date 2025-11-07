# TDL Documentation

Complete guide to the Temporal Deterministic Language.

## Quick Links

- 🚀 **[Getting Started](./getting_started.md)** - Write your first TDL program
- 📖 **[Language Reference](./language_reference.md)** - Complete syntax documentation
- 🔗 **[API Reference](./api_reference.md)** - Built-in functions and constructs
- ⏰ **[Clock Modes](./clock_modes.md)** - Frequency-based and max-speed clocks
- ⚙️ **[Parallelism Guide](./parallelism_guide.md)** - Understanding deterministic concurrency

## What is TDL?

TDL is a **hardware description language for software** that solves the fundamental problem of concurrent programming: **how to write deterministic, race-free concurrent code without manual synchronization**.

**Key property:** Every program produces identical results every execution, with zero race conditions.

## Why TDL?

### The Problem

Traditional concurrent programming (Java, C++, Go, Rust) has unsolved issues:

```
Race conditions   ✗ Unavoidable
Non-determinism   ✗ Every run different
Locks/mutexes     ✓ Required but error-prone
Deadlocks         ✗ Always possible
Debugging         ✗ Nearly impossible
```

### The TDL Solution

```
Determinism       ✓ Guaranteed
Race-free         ✓ By design
Synchronization   ✓ Automatic
Deadlocks         ✓ Impossible
Debugging         ✓ Reproducible
```

## Core Concepts

### 1. Clocks

Synchronize execution to deterministic time points.

```tdl
clock sys = 100hz;    // 100 ticks per second
clock fast;           // Max speed, no delays
```

### 2. Processes

Deterministic concurrent units with isolated state.

```tdl
proc worker(chan<int> in, chan<int> out) {
  on sys.tick {
    static counter: int = 0;
    counter = counter + 1;
    out.send(counter);
  }
}
```

### 3. Channels

FIFO message passing with ordering guarantees.

```tdl
channel.send(value);  // Deterministic send
```

### 4. Functions

Reusable logic with parameters and return types.

```tdl
func fibonacci(int n) -> int {
  if (n <= 1) { return n; }
  return fibonacci(n - 1) + fibonacci(n - 2);
}
```

## Example Program

```tdl
// Define clocks
clock sys = 10hz;

// Define functions
func double_value(int x) -> int {
  return x * 2;
}

// Define processes
proc producer(chan<int> out) {
  on sys.tick {
    static i: int = 0;
    println(i);
    out.send(i);
    i = i + 1;
  }
}

proc consumer(chan<int> in) {
  on sys.tick {
    println(100);
  }
}
```

## Documentation Map

```
docs/
├── index.md (this file)
├── getting_started.md       ← Start here
├── language_reference.md    ← Complete syntax
├── api_reference.md         ← Functions/builtins
├── clock_modes.md          ← Clock configuration
└── parallelism_guide.md    ← Concurrency concepts
```

## Learning Path

1. **New to TDL?**
   - Start with [Getting Started](./getting_started.md)
   - Run the examples
   - Try modifying them

2. **Ready for more?**
   - Read [Language Reference](./language_reference.md)
   - Understand the syntax
   - Look up specific constructs

3. **Understanding parallelism?**
   - Study [Parallelism Guide](./parallelism_guide.md)
   - Compare with traditional threading
   - See why TDL is different

4. **Fine-tuning your code?**
   - Read [Clock Modes](./clock_modes.md)
   - Choose frequency vs max-speed
   - Optimize for your use case

5. **Reference looking up?**
   - Use [API Reference](./api_reference.md)
   - Quick syntax lookups
   - Type information

## Common Tasks

### Run an Example

```bash
cd /Users/duke/Code/tick
./build/bin/tdl examples/fibonacci.tdl
```

### Create a New Program

1. Create `program.tdl`
2. Add clock, processes, functions
3. Run: `./build/bin/tdl program.tdl`

### Debug Output

Add `println()` statements to trace execution:

```tdl
proc worker() {
  on sys.tick {
    println("Processing");
  }
}
```

### Check Statistics

All runs print determinism statistics:

```
=== Statistics ===
Ticks executed: 10
Average slack: 19.95 ms
```

### Choose Clock Speed

- Real-time: `clock sys = 100hz;`
- Fast simulation: `clock fast;`
- Slow analysis: `clock debug = 1hz;`

## Architecture

```
TDL Source Code (.tdl)
    ↓ [Lexer]
Tokens
    ↓ [Parser]
Abstract Syntax Tree
    ↓ [Code Generator]
C++17 with Embedded Runtime
    ↓ [g++ Compiler]
Executable Binary
    ↓ [Auto-Execute]
Output + Statistics
```

## Supported Platforms

- **macOS** (x86-64, Apple Silicon)
- **Linux** (x86-64)
- **Windows** (WSL2 recommended)

Requires: C++17 compiler (g++, clang, MSVC)

## Examples Included

| Example | Purpose | Complexity |
|---------|---------|------------|
| minimal.tdl | Clock synchronization | Beginner |
| fibonacci.tdl | Recursive functions | Beginner |
| counter.tdl | Static variables | Beginner |
| max_speed.tdl | Max-speed execution | Beginner |
| deterministic_accumulator.tdl | Parallel processes | Intermediate |
| deterministic_pipeline.tdl | Producer-consumer | Intermediate |
| pipeline.tdl | Multi-stage processing | Intermediate |
| dsp.tdl | DSP pipeline | Advanced |

## Troubleshooting

### Parse Error

**Problem:** `Parse error at line X`

**Solution:** Check [Language Reference](./language_reference.md) for syntax

### Compilation Error (C++)

**Problem:** `C++ compilation failed`

**Solution:** Check syntax is valid TDL

### No Output

**Problem:** Program runs but prints nothing

**Solution:** Add `println()` to processes

### Unexpected Results

**Problem:** Output doesn't match expectations

**Solution:** Check if using `static` for persistent state

## Frequently Asked Questions

### Q: Can I read from channels?

A: Not yet. Currently send-only. Receive is implicit.

### Q: Can I use arrays?

A: Not yet. Use multiple variables or functions.

### Q: Can I use for loops?

A: Not yet. Use `while` loops or recursion.

### Q: How do I create threads?

A: Use `proc` definitions. They run in deterministic parallel automatically.

### Q: Is TDL compiled or interpreted?

A: Compiled. TDL → C++17 → Binary (via g++)

### Q: How fast is TDL?

A: Max-speed clocks run at CPU speed. Zero synchronization overhead.

### Q: Can I use existing C++ libraries?

A: Not directly. TDL is self-contained.

### Q: Is TDL production-ready?

A: Great for research and learning. Production use requires more features.

## Getting Help

1. **Read the docs** - Most answers are in [Language Reference](./language_reference.md)
2. **Check examples** - Run and modify existing programs
3. **Start simple** - Write minimal programs first, then expand
4. **Use statistics** - Verify determinism with slack metrics

## Project Status

- ✅ Lexer, Parser, Code Generator complete
- ✅ Runtime (Clock, Channel, Scheduler) complete
- ✅ Core language features working
- ⏳ Array support (planned)
- ⏳ For loops (planned)
- ⏳ File I/O (planned)

## Key Achievements

- ✅ True deterministic parallelism
- ✅ Zero race conditions (by design)
- ✅ Automatic synchronization
- ✅ Reproducible execution
- ✅ Complete compiler toolchain
- ✅ Multiple working examples

## Next Steps

1. **Start:** [Getting Started](./getting_started.md)
2. **Learn:** [Language Reference](./language_reference.md)
3. **Understand:** [Parallelism Guide](./parallelism_guide.md)
4. **Build:** Your own programs
5. **Optimize:** Using [Clock Modes](./clock_modes.md)

---

**TDL: Where deterministic parallelism is not a hope, but a guarantee.**
