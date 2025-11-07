# TDL Language Reference

Complete syntax and semantics guide for TDL programming.

## Overview

TDL (Temporal Deterministic Language) is a hardware description language for software. Write concurrent programs that are guaranteed to produce identical results every execution with zero race conditions.

## Language Syntax

### Comments

```tdl
// Single-line comment
```

### Clock Declarations

Define clocks that synchronize program execution.

**Frequency-based clock** (enforces timing):
```tdl
clock sys = 100hz;
clock tick = 50hz;
```

**Max-speed clock** (runs as fast as possible):
```tdl
clock fast;
```

**Available frequencies:**
- Any positive integer followed by `hz`
- Examples: `1hz`, `10hz`, `100hz`, `1000hz`
- Default (no frequency): 999999 Hz (max speed)

### Process Declarations

Define deterministic concurrent units.

```tdl
proc name(parameters) {
  body
}
```

**Parameters** (all optional):
```tdl
proc worker(chan<int> input, chan<int> output) { }
proc sensor(chan<float> data) { }
proc simple() { }
```

**Process body:**
```tdl
proc producer(chan<int> out) {
  on sys.tick {
    static counter: int = 0;
    counter = counter + 1;
    println(counter);
    out.send(counter);
  }
}
```

### Function Declarations

Define reusable logic.

```tdl
func name(parameters) -> return_type {
  body
}
```

**Examples:**
```tdl
func add(int a, int b) -> int {
  return a + b;
}

func fibonacci(int n) -> int {
  if (n <= 1) {
    return n;
  }
  return fibonacci(n - 1) + fibonacci(n - 2);
}

func greet(string name) -> string {
  return name;
}
```

### Variable Declarations

**Local variables** (exist only during tick):
```tdl
let x: int = 0;
let name: string = "value";
let flag: bool = true;
let value: double = 3.14;
```

**Static variables** (persist across ticks):
```tdl
static counter: int = 0;
static accumulator: double = 0.0;
```

### On-Clock Blocks

Execute code synchronously every clock tick.

```tdl
proc worker(chan<int> out) {
  on sys.tick {
    // Code runs every tick, deterministically
    static i: int = 0;
    println(i);
    i = i + 1;
  }
}
```

Multiple on-clock blocks in one process:
```tdl
proc multi(chan<int> out) {
  on sys.tick {
    println("A");
  }
  
  on sys.tick {
    println("B");
  }
}
```

### Channel Operations

**Channel type:**
```tdl
chan<int>
chan<double>
chan<bool>
chan<string>
```

**Send to channel:**
```tdl
out.send(42);
output.send(counter);
result.send(100 * 2);
```

### Control Flow

**If statement:**
```tdl
if (condition) {
  println("true");
}
```

**While loop:**
```tdl
while (counter < 10) {
  counter = counter + 1;
}
```

**Return statement:**
```tdl
func getValue() -> int {
  return 42;
}

func calculate() -> int {
  if (flag) {
    return 100;
  }
  return 0;
}
```

### Expressions

**Arithmetic operators:**
```tdl
x + 5        // Addition
x - 3        // Subtraction
x * 2        // Multiplication
x / 4        // Division
x % 3        // Modulo
```

**Comparison operators:**
```tdl
x == 5       // Equal
x != 5       // Not equal
x < 10       // Less than
x > 3        // Greater than
x <= 10      // Less than or equal
x >= 0       // Greater than or equal
```

**Logical operators:**
```tdl
a && b       // AND
a || b       // OR
!a           // NOT
```

**Assignment:**
```tdl
x = 10;
counter = counter + 1;
name = "value";
```

### Built-in Functions

**println** - Print to output:
```tdl
println(42);           // Integer
println(3.14);         // Double
println("hello");      // String
println(true);         // Boolean
```

## Data Types

| Type | Example | Size |
|------|---------|------|
| `int` | `42` | 32-bit signed integer |
| `double` | `3.14` | 64-bit floating point |
| `bool` | `true`, `false` | Boolean |
| `string` | `"hello"` | Character sequence |
| `chan<T>` | `chan<int>` | Channel of type T |

## Execution Model

### Clock Ticks

Every clock tick, all processes synchronized to that clock execute:

```
Tick 0: [Process A] [Process B] [Process C] (parallel)
Tick 1: [Process A] [Process B] [Process C] (parallel)
Tick 2: [Process A] [Process B] [Process C] (parallel)
...
```

### State Management

**Local variables** (reset each tick):
```tdl
on sys.tick {
  let temp: int = 0;  // Fresh every tick
}
```

**Static variables** (persist across ticks):
```tdl
on sys.tick {
  static count: int = 0;  // Carries over to next tick
  count = count + 1;
}
```

### Process Isolation

- Each process has only local state
- No shared memory between processes
- Only communication: channels

### Deterministic Scheduling

- All processes execute in deterministic order
- Same order every execution
- Identical output every run

## Program Structure

Typical TDL program:

```tdl
// 1. Clock declarations
clock sys = 50hz;
clock fast;

// 2. Function definitions (optional)
func helper(int x) -> int {
  return x * 2;
}

// 3. Process definitions
proc producer(chan<int> out) {
  on sys.tick {
    static i: int = 0;
    out.send(i);
    i = i + 1;
  }
}

proc consumer(chan<int> in) {
  on fast.tick {
    let value: int = 42;
    println(value);
  }
}
```

## Scope and Lifetime

### Local Variables

```tdl
on sys.tick {
  let x: int = 0;  // Born here
  let y: int = x + 1;
}  // x, y destroyed here
```

Next tick:
```tdl
on sys.tick {
  let x: int = 0;  // New x, different from previous
}
```

### Static Variables

```tdl
static counter: int = 0;

on sys.tick {
  counter = counter + 1;  // Incremented every tick
  println(counter);       // 1, 2, 3, 4, ...
}
```

### Function Scope

```tdl
func calculate() -> int {
  let temp: int = 42;  // Local to function
  return temp;
}
```

## Best Practices

### 1. Use Static for State

```tdl
// Good: State persists across ticks
proc counter(chan<int> out) {
  on sys.tick {
    static i: int = 0;
    i = i + 1;
    out.send(i);
  }
}

// Avoid: State lost every tick
proc bad(chan<int> out) {
  on sys.tick {
    let i: int = 0;  // Always 0
    out.send(i);
  }
}
```

### 2. Use Functions for Reusable Logic

```tdl
// Good
func double(int x) -> int { return x * 2; }

proc worker(chan<int> out) {
  on sys.tick {
    let result: int = double(42);
    out.send(result);
  }
}

// Avoid: Duplicated logic
proc worker1(chan<int> out) { on sys.tick { out.send(84); } }
proc worker2(chan<int> out) { on sys.tick { out.send(84); } }
```

### 3. Use Appropriate Clock Frequency

```tdl
// Real-time systems
clock sys = 100hz;    // 100 ticks/sec

// Batch processing
clock fast;           // No delays

// Simulation
clock sim = 1hz;      // 1 tick/sec
```

### 4. Name Processes Clearly

```tdl
// Good
proc temperature_sensor(chan<double> out) { }
proc data_processor(chan<double> in, chan<int> out) { }

// Avoid
proc p1(chan<double> out) { }
proc p2(chan<double> in, chan<int> out) { }
```

## Limitations

### Not Supported (Yet)

- ❌ Array declarations
- ❌ For loops (use while + recursion)
- ❌ Break/continue
- ❌ Structs/records
- ❌ File I/O
- ❌ Generics/templates
- ❌ Pattern matching
- ❌ Channel receive (read-only)

### Notes

- All execution is deterministic
- No undefined behavior (by design)
- No memory safety issues (statically checked)
- No data races (guaranteed)

## Next: See [Getting Started](./getting_started.md)
