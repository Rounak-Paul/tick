# TDL Language Reference

Complete syntax and semantics guide for TDL (Temporal Deterministic Language) programming.

## Overview

TDL is a deterministic programming language with automatic parallelization. The compiler analyzes data dependencies between statements and executes independent operations in parallel, while maintaining complete determinism.

## Language Basics

### Comments

```tdl
// Single-line comment
```

### Data Types

TDL supports the following data types:

```tdl
int     // Integer values
bool    // Boolean: true or false
```

### Variable Declarations

**Local variables** (exist only during function execution):
```tdl
let x: int = 0;
let flag: bool = true;
let y: int = x + 5;
```

**Static variables** (persist across function calls):
```tdl
static counter: int = 0;
static accumulator: int = 0;
```

Static variables are initialized once and maintain their value across multiple invocations.

## Function Declarations

Define reusable blocks of code.

```tdl
func name(parameter1: type, parameter2: type) -> return_type {
  // function body
  return value;
}
```

### Parameters and Return Types

```tdl
func add(int a, int b) -> int {
  return a + b;
}

func square(int x) -> int {
  let result: int = x * x;
  return result;
}

func is_positive(int x) -> bool {
  return x > 0;
}
```

Functions with no parameters:

```tdl
func get_default() -> int {
  return 42;
}
```

Functions with no return value use `main()` convention:

```tdl
func main() {
  println(10);
}
```

## Control Flow

### If/Else Statements

```tdl
if (condition) {
  // executed if condition is true
} else if (other_condition) {
  // executed if other_condition is true
} else {
  // executed otherwise
}
```

Example:

```tdl
func max(int a, int b) -> int {
  if (a > b) {
    return a;
  } else {
    return b;
  }
}
```

### While Loops

```tdl
while (condition) {
  // loop body executes while condition is true
}
```

Example:

```tdl
func factorial(int n) -> int {
  let result: int = 1;
  let i: int = 1;
  while (i <= n) {
    result = result * i;
    i = i + 1;
  }
  return result;
}
```

## Expressions and Operators

### Arithmetic Operators

```tdl
x + y       // Addition
x - y       // Subtraction
x * y       // Multiplication
x / y       // Division
```

### Comparison Operators

```tdl
x == y      // Equal to
x != y      // Not equal to
x > y       // Greater than
x < y       // Less than
x >= y      // Greater than or equal to
x <= y      // Less than or equal to
```

### Logical Operators

```tdl
x && y      // Logical AND
x || y      // Logical OR
!x          // Logical NOT
```

### Operator Precedence

Operations follow standard mathematical precedence:

1. Unary operators: `!`, `-`
2. Multiplication/Division: `*`, `/`
3. Addition/Subtraction: `+`, `-`
4. Comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`
5. Logical AND: `&&`
6. Logical OR: `||`

## Automatic Parallelization

### How It Works

TDL automatically detects which statements can run in parallel:

```tdl
func main() {
  let a: int = 1 + 2;      // Can run in parallel
  let b: int = 3 + 4;      // Can run in parallel (independent of a)
  let c: int = a + b;      // Must wait for a and b
  println(c);
}
```

The compiler:
1. Analyzes which variables each statement reads and writes
2. Groups statements into "layers" where statements in a layer don't depend on each other
3. Executes each layer in parallel using multiple threads
4. Maintains deterministic execution order between layers

### Writing Parallelizable Code

Statements are independent if they:
- Don't read variables written by other statements
- Don't write to variables read by other statements

Independent statements:
```tdl
func compute() {
  let x: int = 10 + 20;     // Layer 1
  let y: int = 30 + 40;     // Layer 1
  let z: int = 50 + 60;     // Layer 1
  let sum: int = x + y + z; // Layer 2 (depends on Layer 1)
}
```

Dependent statements (executed sequentially):
```tdl
func accumulate() {
  let total: int = 0;       // Layer 1
  total = total + 5;        // Layer 2 (depends on total)
  total = total + 10;       // Layer 3 (depends on updated total)
  println(total);
}
```

## Built-in Functions

### I/O Functions

```tdl
println(value)   // Print value followed by newline
```

Examples:
```tdl
func main() {
  println(42);           // Output: 42
  let x: int = 10 + 20;
  println(x);            // Output: 30
}
```

## Program Structure

A TDL program consists of:

1. Function declarations
2. A `main()` function (entry point)

```tdl
func helper(int x) -> int {
  return x * 2;
}

func main() {
  let result: int = helper(21);
  println(result);  // Output: 42
}
```

The `main()` function is the entry point and executes automatically when the program runs.

## Scope and Variable Lifetime

**Global scope:** Functions are accessible throughout the program.

**Function scope:** Variables declared with `let` exist only within that function.

```tdl
func process() {
  let x: int = 10;      // x exists only in process()
  println(x);
}

func main() {
  // x does not exist here
  process();
}
```

**Static variables:** Maintain their value across function calls.

```tdl
func counter() -> int {
  static count: int = 0;
  count = count + 1;
  return count;
}

func main() {
  println(counter());    // Output: 1
  println(counter());    // Output: 2
  println(counter());    // Output: 3
}
```

## Examples

### Example 1: Fibonacci

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

### Example 2: Deterministic Counter

```tdl
func next() -> int {
  static value: int = 0;
  value = value + 1;
  return value;
}

func main() {
  println(next());    // Output: 1
  println(next());    // Output: 2
  println(next());    // Output: 3
}
```

### Example 3: Parallel Computation

```tdl
func main() {
  let a: int = 100 + 200;     // Runs in parallel
  let b: int = 300 + 400;     // Runs in parallel
  let c: int = 500 + 600;     // Runs in parallel
  
  let total: int = a + b + c; // Waits for a, b, c
  println(total);             // Output: 2100
}
```

## Determinism Guarantee

Every TDL program produces identical output across all executions. This is guaranteed by:

1. **No randomness:** No random functions or non-deterministic operations
2. **No threading conflicts:** Parallelization happens automatically without race conditions
3. **Deterministic ordering:** Independent operations run in parallel, but results are always combined in the same order
4. **Static variables:** Always initialize to the same value and maintain state consistently

This makes TDL ideal for:
- Reproducible scientific computation
- Reliable data processing pipelines
- Deterministic simulation
- Auditable algorithms
