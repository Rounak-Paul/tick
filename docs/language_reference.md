# TDL Language Reference

Complete syntax and semantics guide for TDL (Temporal Deterministic Language) programming.

## Overview

TDL is a **strongly typed**, deterministic programming language with automatic parallelization. Every variable, function parameter, and function return value must have an explicit type annotation. The compiler performs comprehensive type checking to catch errors before execution and analyzes data dependencies between statements to execute independent operations in parallel, while maintaining complete determinism.

## Language Basics

### Comments

```tdl
// Single-line comment
```

### Data Types

TDL supports the following primitive data types with strong typing:

```tdl
int       // 32-bit signed integer values
float     // 32-bit floating-point numbers
double    // 64-bit high-precision floating-point numbers
bool      // Boolean: true or false
string    // Text strings
void      // No value (function return type only)
```

All variables and function parameters must have explicit type declarations. Type mismatches are compile-time errors.

### Type Safety

TDL enforces strict type checking:

```tdl
let x: int = 42;        // OK: integer value
let y: int = 3.14;      // ERROR: cannot assign float to int
let z: float = 3.14;    // OK: float value
let name: string = "hello";  // OK: string value
```

### Variable Declarations

**Local variables** (exist only during function execution):
```tdl
let x: int = 0;
let flag: bool = true;
let y: int = x + 5;
let value: float = 2.71;
let message: string = "Hello";
```

**Static variables** (persist across function calls):
```tdl
static counter: int = 0;
static accumulator: double = 0.0;
```

Static variables are initialized once and maintain their value across multiple invocations. Type annotations are required.

## Type Compatibility Rules

### Numeric Type Conversions

Numeric types (int, float, double) can be used interchangeably in arithmetic operations:

```tdl
func mixed_arithmetic() -> double {
  let i: int = 10;
  let f: float = 2.5;
  let d: double = 3.14159;
  
  // Automatic type promotion in operations
  let result: double = i + f + d;  // Result is double
  return result;
}
```

The result type follows these rules:
- If any operand is `double`, the result is `double`
- Else if any operand is `float`, the result is `float`
- Else the result is `int`

### No Implicit Type Conversions

Assignment requires exact type match or valid numeric conversion:

```tdl
let x: int = 5;
let y: double = x;      // ERROR: cannot assign int to double
let z: double = 5.0;    // OK: explicit double literal
```

## Function Declarations

Define reusable blocks of code with required type annotations.

```tdl
func name(parameter1: type, parameter2: type) -> return_type {
  // function body
  return value;
}
```

### Parameters and Return Types

All function parameters and return types must be explicitly annotated:

```tdl
func add(a: int, b: int) -> int {
  return a + b;
}

func square(x: float) -> float {
  let result: float = x * x;
  return result;
}

func is_positive(x: int) -> bool {
  return x > 0;
}

func format_message(name: string, count: int) -> string {
  return name + " has " + count + " items";
}
```

Functions with no parameters:

```tdl
func get_default() -> int {
  return 42;
}
```

Functions with no return value use explicit `void` return type:

```tdl
func print_info(message: string) -> void {
  println(message);
}

// Or implicitly (if no return statement):
func main() {
  println(10);
}
```

### Type Checking on Function Calls

Function calls are type-checked at compile time:

```tdl
func multiply(x: int, y: int) -> int {
  return x * y;
}

// Valid calls
multiply(2, 3);         // OK
multiply(2, 3 + 4);     // OK

// Type errors (caught at compile time)
multiply(2.5, 3);       // ERROR: float passed where int expected
multiply(2);            // ERROR: missing required argument
multiply("hello", 3);   // ERROR: string passed where int expected
```

## Control Flow

### If/Else Statements

Conditions must be boolean expressions:

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
func max(a: int, b: int) -> int {
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
func factorial(n: int) -> int {
  let result: int = 1;
  let i: int = 1;
  while (i <= n) {
    result = result * i;
    i = i + 1;
  }
  return result;
}
```

## Type Checking and Error Detection

### Compile-Time Type Validation

TDL performs comprehensive type checking at compile time:

1. **Variable Initialization**: Initializer type must match variable type
   ```tdl
   let x: int = 5;        // OK
   let y: int = 5.5;      // ERROR: float assigned to int
   ```

2. **Operator Type Constraints**:
   ```tdl
   let a: int = 5;
   let b: bool = true;
   let c: int = a + b;    // ERROR: cannot add int and bool
   let d: bool = a && b;  // ERROR: && requires bool operands
   ```

3. **Function Argument Types**:
   ```tdl
   func process(x: int) -> string {
     return "value";
   }
   
   process(42);           // OK
   process(3.14);         // ERROR: float passed to int parameter
   process("text");       // ERROR: string passed to int parameter
   ```

4. **Return Type Validation**:
   ```tdl
   func get_count() -> int {
     return 42;           // OK
     return 3.14;         // ERROR: float returned from int function
     return "count";      // ERROR: string returned from int function
   }
   ```

### Operator Type Requirements

- **Arithmetic** (`+`, `-`, `*`, `/`): Requires numeric operands (int, float, double)
- **Modulo** (`%`): Requires integer operands
- **Comparison** (`==`, `!=`, `<`, `<=`, `>`, `>=`): Requires same types (or numeric types)
- **Logical** (`&&`, `||`): Requires boolean operands
- **Logical NOT** (`!`): Requires boolean operand
- **Unary minus** (`-`): Requires numeric operand

### Type Errors

The compiler catches many type errors before execution:

```tdl
let x: int = "hello";           // ERROR: type mismatch in assignment
let y: bool = 5;                // ERROR: int assigned to bool
let z: int = x / true;          // ERROR: cannot divide int by bool
if (42) { println("ok"); }      // ERROR: if condition must be bool
```

## Expressions and Operators

### Arithmetic Operators

All operands must be numeric (int, float, or double):

```tdl
x + y       // Addition
x - y       // Subtraction
x * y       // Multiplication
x / y       // Division
x % y       // Modulo (integer only)
```

Examples:

```tdl
let a: int = 10;
let b: int = 3;
let sum: int = a + b;           // 13
let product: int = a * b;       // 30
let quotient: int = a / b;      // 3
let remainder: int = a % b;     // 1

let x: float = 3.14;
let y: float = 2.0;
let result: float = x / y;      // 1.57
```

### Comparison Operators

Operands must be compatible types (same type or both numeric):

```tdl
x == y      // Equal to (returns bool)
x != y      // Not equal to (returns bool)
x > y       // Greater than (returns bool)
x < y       // Less than (returns bool)
x >= y      // Greater than or equal to (returns bool)
x <= y      // Less than or equal to (returns bool)
```

Examples:

```tdl
let a: int = 5;
let b: int = 3;
let equal: bool = a == b;       // false
let greater: bool = a > b;      // true

let name1: string = "Alice";
let name2: string = "Bob";
let same: bool = name1 == name2; // false
```

### Logical Operators

All operands must be boolean:

```tdl
x && y      // Logical AND (requires bool operands)
x || y      // Logical OR (requires bool operands)
!x          // Logical NOT (requires bool operand)
```

Examples:

```tdl
let a: bool = true;
let b: bool = false;
let both: bool = a && b;        // false
let either: bool = a || b;      // true
let not_a: bool = !a;           // false
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
