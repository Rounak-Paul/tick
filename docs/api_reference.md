# API Reference

Complete reference for TDL's built-in functions and constructs.

## Clock API

### Declaration

```tdl
clock name;              // Max-speed clock
clock name = NHz;        // Frequency-based clock
```

### Examples

```tdl
clock sys = 100hz;
clock fast;
clock motor = 1000hz;
```

### Properties

- **name**: Clock identifier
- **frequency**: Ticks per second (if specified)
- **period**: 1000ms / frequency (if specified)

### Usage in Code

```tdl
proc worker() {
  on sys.tick {
    // Execute every sys.tick
  }
}
```

## Process API

### Declaration

```tdl
proc name() { }
proc name(params) { }
```

### Examples

```tdl
proc producer(chan<int> out) { }
proc worker(chan<int> in, chan<int> out) { }
proc consumer() { }
```

### Parameters

```tdl
chan<int>       // Integer channel
chan<double>    // Double channel
chan<bool>      // Boolean channel
chan<string>    // String channel
```

### Body

```tdl
proc example() {
  on clock.tick {
    // Statements
  }
}
```

## Function API

### Declaration

```tdl
func name() { }
func name(params) -> type { }
func name() -> type { return value; }
```

### Examples

```tdl
func add(int a, int b) -> int {
  return a + b;
}

func fibonacci(int n) -> int {
  if (n <= 1) { return n; }
  return fibonacci(n - 1) + fibonacci(n - 2);
}

func greet() {
  println("Hello");
}
```

### Return Types

```tdl
int     // 32-bit signed integer
double  // 64-bit floating point
bool    // Boolean
string  // Character string
```

### Parameters

```tdl
func process(int value, double scale, bool flag, string name) -> int { }
```

## Variable API

### Declaration

```tdl
let name: type = value;
let name: type;

static name: type = value;
static name: type;
```

### Examples

```tdl
let x: int = 0;
let flag: bool = true;
let text: string = "value";
let scale: double = 3.14;

static counter: int = 0;
static accumulator: double = 0.0;
```

### Scope

**let (Local):**
- Exists only during current tick
- Fresh value every tick
- Cannot be accessed from other ticks

**static (Persistent):**
- Survives across ticks
- Initialized once, persists
- Access in next tick has previous value

## Channel API

### Types

```tdl
chan<int>
chan<double>
chan<bool>
chan<string>
```

### Creation

```tdl
proc producer(chan<int> out) {
  on sys.tick {
    out.send(value);
  }
}

proc consumer(chan<int> in) {
  on sys.tick {
    // Implicit receive via on-clock synchronization
  }
}
```

### Operations

**Send:**
```tdl
channel.send(value);
```

**Receive (Implicit):**
```tdl
// Handled automatically by runtime
// Not directly exposed to user code
```

### Properties

- **Type:** `chan<T>`
- **FIFO:** First-in, first-out ordering
- **Bounded:** Fixed size queue
- **Thread-safe:** No race conditions

## Statement API

### On-Clock Statement

```tdl
on clock.tick {
  statements;
}
```

### If Statement

```tdl
if (condition) {
  statements;
}
```

### While Loop

```tdl
while (condition) {
  statements;
}
```

### Return Statement

```tdl
return;
return value;
```

### Expression Statement

```tdl
expression;
```

### Variable Declaration Statement

```tdl
let x: int = 0;
static counter: int = 0;
```

## Expression API

### Literals

```tdl
42              // Integer literal
3.14            // Double literal
true, false     // Boolean literals
"string"        // String literal
```

### Identifiers

```tdl
variable_name
function_name
clock_name
channel_name
```

### Arithmetic Operators

```tdl
x + y           // Addition
x - y           // Subtraction
x * y           // Multiplication
x / y           // Division
x % y           // Modulo (remainder)
```

### Comparison Operators

```tdl
x == y          // Equal
x != y          // Not equal
x < y           // Less than
x > y           // Greater than
x <= y          // Less than or equal
x >= y          // Greater than or equal
```

### Logical Operators

```tdl
a && b          // Logical AND
a || b          // Logical OR
!a              // Logical NOT
```

### Assignment Operators

```tdl
x = value       // Assignment
```

### Function Call

```tdl
func_name()
func_name(arg1, arg2)
func_name(arg1, arg2, arg3)
```

### Binary Operations

```tdl
(left op right)
```

### Unary Operations

```tdl
!flag
-value
```

## Built-in Functions

### println

Print values to stdout.

**Signature:**
```tdl
println(int)
println(double)
println(bool)
println(string)
```

**Examples:**
```tdl
println(42);
println(3.14);
println(true);
println("hello");
```

**Output:**
```
42
3.14
1
hello
```

(Note: bool prints as 1 for true, 0 for false)

## Type System

### Supported Types

| Type | Range | Example |
|------|-------|---------|
| `int` | -2,147,483,648 to 2,147,483,647 | `42` |
| `double` | IEEE 754 64-bit | `3.14` |
| `bool` | true/false | `true` |
| `string` | UTF-8 string | `"hello"` |
| `chan<T>` | Channel of type T | `chan<int>` |

### Type Coercion

- No implicit type coercion
- All types must match exactly
- Cast not supported (use values of correct type)

## Scope Rules

### Global Scope

```tdl
clock sys = 100hz;       // Clock (global)
func helper() -> int { } // Function (global)
proc worker() { }        // Process (global)
```

### Function Scope

```tdl
func example() -> int {
  let x: int = 0;        // Function-scoped
  return x;
}
```

### Process Scope

```tdl
proc worker() {
  on sys.tick {
    let x: int = 0;      // Tick-scoped (lost after tick)
    static y: int = 0;   // Process-scoped (persistent)
  }
}
```

## Execution Model

### Program Execution

1. Parse all declarations (clocks, functions, processes)
2. For each tick (0 to 9 by default):
   - Execute all on-clock blocks
   - For each clock, find all processes on that clock
   - Execute them in deterministic order
   - Synchronize (wait for slowest to complete)
   - Sleep if frequency-based clock
   - Advance to next tick

### Deterministic Guarantees

- Same clock frequency → same execution order
- Same processes → same output
- Same input → identical results
- No race conditions
- No deadlocks
- No undefined behavior

## Constants

### Keywords

```tdl
clock, proc, func, let, static, on, if, while, return, println,
true, false, hz, chan
```

### Reserved Words

Do not use as identifiers: see above keywords

### Special Names

```tdl
tick    // Clock event (e.g., sys.tick)
send    // Channel method (e.g., channel.send())
```

## Size Limits

| Item | Limit | Notes |
|------|-------|-------|
| Channel queue | 4 items | Fixed size |
| Process count | Unlimited | Practical: thousands |
| Variable count | Unlimited | Practical: millions |
| String length | Unlimited | Limited by memory |
| Recursion depth | OS stack | Limited by available memory |

## Performance Notes

- **Max-speed clock:** ~microseconds per tick
- **Frequency-based:** ~10ms sleep overhead per tick
- **Channel send:** Microseconds (bounded queue)
- **Function call:** Nanoseconds (inlined by C++ compiler)
- **Static variable access:** Nanoseconds

## Error Handling

TDL does not support exception handling. All errors result in:
- **Compile-time:** Error message and non-zero exit code
- **Runtime:** Program terminates (C++ runtime error)

## Next: See [Examples](../examples/)
