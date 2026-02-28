# Tick Language Features

## Overview

Tick is a statically-typed, compiled programming language that transpiles to C and compiles to native code via GCC. It supports classes, concurrency through signals and events, and a module system.

---

## Type System

### Primitive Types

| Type | Description | C Equivalent |
|------|-------------|--------------|
| `int` | 32-bit signed integer | `int` |
| `float` | 64-bit floating point | `double` |
| `double` | 64-bit floating point | `double` |
| `bool` | Boolean (`true`/`false`) | `bool` |
| `string` | String literal | `char*` |

### Array Types

Arrays are declared with `[]` suffix on the base type. Array literals use square brackets.

```tick
var numbers : int[] = [1, 2, 3, 4, 5];
var flags : bool[] = [true, false, true];
var values : float[] = [1.5, 2.5, 3.5];
```

Array elements are accessed via index notation:

```tick
var x : int = numbers[0];
numbers[2] = 99;
```

### Constants

Variables declared with `const` cannot be reassigned.

```tick
const pi : float = 3.14159;
const max_size : int = 1024;
const enabled : bool = true;
```

---

## Variables

Variables are explicitly typed with the syntax `var name : type = value`.

```tick
var x : int = 10;
var name : string = "tick";
var active : bool = true;
var ratio : double = 2.718;
```

Variables default to `0` if no initializer is provided.

---

## Functions

Functions are declared with `func`, explicit parameter types, and a return type.

```tick
func add(a : int, b : int) : int {
    return a + b;
}

func greet() : void {
    println("hello");
}
```

### Recursion

Full recursive function support:

```tick
func factorial(n : int) : int {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}
```

### Built-in Functions

| Function | Description |
|----------|-------------|
| `print(expr)` | Print a value (type-aware: int, float, bool, string) |
| `println(expr)` | Print a value followed by a newline |

Print is type-aware: integers use `%d`, floats/doubles use `%f`, booleans print `true`/`false`, and strings use `%s`.

---

## Operators

### Arithmetic
`+`, `-`, `*`, `/`, `%`

### Comparison
`==`, `!=`, `<`, `>`, `<=`, `>=`

### Logical
`&&`, `||`, `!`

### Compound Assignment
`+=`, `-=`, `*=`, `/=`, `%=`

### Prefix Increment/Decrement
`++x`, `--x`

---

## Control Flow

### If / Else

```tick
if (x > 10) {
    println("large");
} else {
    println("small");
}
```

Nested if/else chains:

```tick
if (x > 100) {
    return 3;
} else {
    if (x > 50) {
        return 2;
    } else {
        return 1;
    }
}
```

### While Loop

```tick
var i : int = 0;
while (i < 10) {
    println(i);
    i = i + 1;
}
```

### For Loop

C-style for loops with initializer, condition, and increment:

```tick
for (var i : int = 0; i < 10; i = i + 1) {
    println(i);
}
```

For loop variables are scoped to the loop body and do not leak into the enclosing scope.

### Break

Exit early from any loop:

```tick
while (true) {
    if (done) {
        break;
    }
}
```

### Continue

Skip to the next iteration of a loop:

```tick
for (var i : int = 0; i < 10; i = i + 1) {
    if (i % 2 == 0) {
        continue;
    }
    println(i);
}
```

---

## Classes

Classes support fields, constructors, and methods. Instances are heap-allocated.

```tick
class Point {
    var x : int = 0;
    var y : int = 0;

    func Point(px : int, py : int) : void {
        self.x = px;
        self.y = py;
    }

    func distance_from_origin() : int {
        return self.x * self.x + self.y * self.y;
    }
}
```

### Construction

Calling the class name as a function invokes the constructor:

```tick
var p : Point = Point(3, 4);
```

### Member Access

Fields and methods are accessed with `.` notation:

```tick
var x : int = p.x;
var d : int = p.distance_from_origin();
```

### Self Reference

Inside methods, `self` refers to the current instance (transpiles to a `self` pointer in C).

### Multiple Instances

Each instance maintains independent state:

```tick
var a : Point = Point(1, 2);
var b : Point = Point(3, 4);
/* a and b are independent heap objects */
```

---

## Scoping

### Lexical Scoping

Variables follow lexical scoping rules. Inner scopes can shadow outer variables:

```tick
var x : int = 10;
if (true) {
    var x : int = 20;  /* shadows outer x */
    println(x);        /* prints 20 */
}
println(x);            /* prints 10, outer x restored */
```

### Global Variables

Globals declared outside functions are accessible from all functions:

```tick
var counter : int = 0;

func increment() : void {
    counter = counter + 1;
}
```

---

## Concurrency

Tick provides built-in concurrency primitives: signals, events, and processes.

### Signals

Typed, thread-safe message queues for inter-process communication:

```tick
signal data_ready : Signal<int>;
```

Emit a value:

```tick
data_ready.emit(42);
```

Receive blocks until a value is available:

```tick
var value : int = data_ready.recv();
```

### Signal Arrays

Arrays of signals for channel-per-index patterns:

```tick
signal channels : Signal<int>[4];

channels[0].emit(10);
var v : int = channels[0].recv();
```

### Signal Type Parameters

Signals support any type as parameter, including class types and array types:

```tick
signal point_sig : Signal<Point>;
signal data_sig : Signal<int[]>;
```

### Events

Events group processes and execute them concurrently:

```tick
event on_start;
```

### Processes

Processes are functions bound to events that run in separate threads:

```tick
process worker on on_start {
    var result : int = heavy_computation();
    output.emit(result);
}
```

### Event Execution

Trigger all processes bound to an event:

```tick
on_start.execute();
```

### Concurrency Model

- Each process runs in its own pthread
- Signals use mutex + condition variable for thread-safe communication
- `emit` is non-blocking (drops if queue is full, with warning)
- `recv` blocks until data is available
- Signal queue capacity: 1024 entries

---

## Module System

### Import Syntax

Import all exports from a module:

```tick
import * from "math_utils";
```

Import specific names:

```tick
import { add, multiply } from "math_utils";
```

### Module Resolution

Modules are resolved in order:
1. Relative to the current file directory
2. Current working directory
3. `TICK_PATH` environment variable

Module files use the `.tick` extension.

---

## Comments

### Line Comments

```tick
/* this is a comment */
var x : int = 5;
```

### Block Comments

```tick
/* Multi-line
   block comment */
var y : int = 10;

var z : int = /* inline */ 42;
```

---

## Compilation

Tick compiles to C, then invokes GCC to produce native binaries:

```
tick source.tick output_binary
```

The compilation pipeline:
1. **Lexer** - Tokenizes source code
2. **Parser** - Produces an AST via recursive descent
3. **Semantic Analyzer** - Validates scoping, declarations, types, and loop context
4. **C Code Generator** - Transpiles AST to C source
5. **GCC** - Compiles C to native binary with `-O2 -pthread`

### Keep Generated C

```
tick source.tick output_binary --keep-c
```

Saves the generated `.c` file alongside the output.

---

## Semantic Analysis

The compiler performs the following checks:

- **Undeclared identifier detection** - Using a variable or function that hasn't been declared produces an error
- **Duplicate declaration detection** - Redeclaring a global, event, signal, process, or function in the same scope produces an error
- **Scope tracking** - Block-scoped variables are properly tracked and cleaned up; shadowing is supported with correct restoration
- **Loop context validation** - `break` and `continue` outside of loops produce errors
- **Undeclared event references** - Processes referencing undeclared events produce errors
- **Module resolution** - Import failures produce clear error messages

---

## Test Suite

The language ships with 28 test files covering:

| Category | Tests |
|----------|-------|
| Arithmetic | addition, subtraction, multiplication, division, modulo, float arithmetic |
| Arrays | literals, indexing, assignment, sum, float/bool arrays, nested indexing |
| Basic Types | int, float, bool, double assignment and operations |
| Block Comments | inline comments, multi-line comments |
| Chained Access | method calls, array index expressions, variable indexing |
| Class Methods | constructors, mutation, multiple instances, method parameters |
| Classes | construction, member access, methods, stateful methods, class arrays |
| Comparison | all six comparison operators |
| Complex Expressions | precedence, parentheses, mixed operations, chained comparisons |
| Compound Ops | +=, -=, *=, /=, %= operators |
| Constants | global/local const, const with expressions |
| Continue | while continue, for continue, nested continue |
| Control Flow | if, if-else, while, break, continue, nested loops |
| Double Type | double arithmetic, multiplication, subtraction, comparisons |
| Edge Cases | zero, negatives, large numbers, division, empty arrays, false branches |
| Events | simple event, multiple processes, event reuse |
| Fibonacci | recursive and iterative implementations |
| For Loops | sum range, countdown, nested, break in for, loop scoping |
| Functions | calls, float/bool returns, void functions, recursion, nested calls |
| Increment | prefix ++/--, multiple increments/decrements |
| Logical | AND, OR, NOT, complex boolean expressions |
| Nested Control | early return, multi-branch, deeply nested loops with conditionals |
| Recursion | fibonacci, factorial, power, GCD |
| Scoping | global access/modification, local scope, shadowing, nested scope, restoration |
| Signals (basic) | int/bool signals, signal queue, bool false |
| Signals (arrays) | signal array indexing, sum across channels |
| Signals (array types) | int array as signal type parameter |
| Signals (classes) | class instances through signals |

Run the full suite:

```bash
bash tests/run_test_suite.sh
```
