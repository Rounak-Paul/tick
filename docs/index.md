# TDL Documentation

Complete guide to the Temporal Deterministic Language (v1.0.0 Beta).

## Quick Links

- 🚀 **[Getting Started](./getting_started.md)** - Write your first TDL program
- 📖 **[Language Reference](./language_reference.md)** - Complete syntax documentation
- 🔗 **[API Reference](./api_reference.md)** - Built-in functions and constructs
- ⚙️ **[Parallelism Guide](./parallelism_guide.md)** - Understanding automatic parallel execution

## What is TDL?

TDL is a **language for deterministic, automatic parallel computation** that solves the fundamental problem of concurrent programming: **how to write race-free concurrent code without locks, channels, or manual synchronization**.

**Key property:** Every program produces identical results every execution, with zero race conditions, and independent statements run in parallel automatically.

## Why TDL?

### The Problem

Traditional concurrent programming (Java, C++, Go, Rust) has unsolved issues:

```
Race conditions   ✗ Unavoidable
Non-determinism   ✗ Every run different
Locks/mutexes     ✓ Required but error-prone
Deadlocks         ✗ Always possible
Debugging         ✗ Nearly impossible
Manual threading  ✓ Developer burden
```

### The TDL Solution

```
Determinism       ✓ Guaranteed
Race-free         ✓ By design
Automatic parallel ✓ No manual threads/locks
Deadlocks         ✓ Impossible
Debugging         ✓ Reproducible
Simplicity        ✓ No complexity overhead
```

## Core Concepts

### 1. Functions

The primary unit of computation in TDL. Functions compose into programs.

```tdl
func fibonacci(int n) -> int {
  if (n <= 1) { return n; }
  return fibonacci(n - 1) + fibonacci(n - 2);
}

func main() {
  let result: int = fibonacci(10);
  println(result);
}
```

### 2. Automatic Parallelization

Independent statements execute in parallel automatically. The compiler analyzes data dependencies and parallelizes where safe.

```tdl
func main() {
  let x: int = 1 + 2;      // Parallel Layer 1
  let y: int = 3 + 4;      // Parallel Layer 1 (independent of x)
  let z: int = x + y;      // Layer 2 (waits for x and y)
  println(z);              // Layer 3 (waits for z)
}
```

### 3. Static Variables

Maintain state across function calls, enabling deterministic computation.

```tdl
func counter() -> int {
  static count: int = 0;
  count = count + 1;
  return count;
}
```

### 4. Control Flow

Structured programming with if/else and while loops.

```tdl
func max(int a, int b) -> int {
  if (a > b) {
    return a;
  }
  return b;
}
```

## Example Program

```tdl
// Deterministic parallel computation
func main() {
  // These compute independently and run in parallel
  let a: int = 10 + 20;
  let b: int = 30 + 40;
  let c: int = 50 + 60;
  
  // This waits for a, b, c to complete
  let sum: int = a + b + c;
  
  println(sum);  // Output: 210
}
```

When you run this, the TDL compiler:
1. Detects that `a`, `b`, `c` are independent
2. Executes them in parallel on separate threads
3. Waits for all to complete
4. Computes `sum` with the results
5. Produces deterministic output every time

## Documentation Map

```
docs/
├── index.md (this file)
├── getting_started.md       ← Start here
├── language_reference.md    ← Complete syntax
├── api_reference.md         ← Functions/builtins
└── parallelism_guide.md     ← Automatic parallelization
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
   - See how dependency analysis works
   - Learn how to write parallelizable code

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
