# Tick Programming Language

Tick is a statically-typed, compiled programming language that transpiles to C and compiles via GCC. It features value types, typed pointers, function pointers, classes with inheritance, RAII destructors, signals/events for concurrency, and full C interop through `extern` and `link` directives.

## Building the Compiler

```bash
cmake -B build
cmake --build build
```

This produces `./build/tick`.

## Usage

```bash
./build/tick source.tick                # compiles to ./a.out
./build/tick source.tick -o myprogram   # custom output name
./build/tick source.tick --keep-c       # keep the generated .c file
./build/tick source.tick -D MACOS       # define a compile-time symbol
./build/tick source.tick -DMACOS        # same, no space form
```

The compiler pipeline: **Tick source → Lexer → Parser → Semantic Analyzer → C codegen → GCC (`gcc -O2 ... -pthread -lm`)**.

---

## Language Reference

### Entry Point

Every program requires a `main` function returning `i32`. The return value is the process exit code.

```
func main() : i32 {
    println("hello world");
    return 0;
}
```

---

### Variables

```
var x : i32 = 42;
var name : str = "hello";
var flag : b8 = true;
var pi : f64 = 3.14159;
```

Variables are declared with `var`, followed by the name, a colon, the type, and an optional initializer. Uninitialized variables default to zero/null.

```
var x : i32;       // defaults to 0
var p : ptr;       // defaults to null
var s : str;       // defaults to null
```

### Constants

```
const MAX : i32 = 100;
const PI : f64 = 3.14159;
const DEBUG : b8 = true;
const HALF : i32 = MAX / 2;   // expression allowed
```

Constants are declared with `const`. They can be global or local.

### Global Variables

Variables declared outside any function are global and accessible from all functions.

```
var counter : i32 = 0;

func increment() : void {
    counter = counter + 1;
}

func main() : i32 {
    increment();
    println(to_str(counter));
    return 0;
}
```

---

### Type System

| Type | C Equivalent | Size | Description |
|------|-------------|------|-------------|
| `i8` | `int8_t` | 1 byte | 8-bit signed integer |
| `i16` | `int16_t` | 2 bytes | 16-bit signed integer |
| `i32` | `int32_t` | 4 bytes | 32-bit signed integer |
| `i64` | `int64_t` | 8 bytes | 64-bit signed integer |
| `u8` | `uint8_t` | 1 byte | 8-bit unsigned integer |
| `u16` | `uint16_t` | 2 bytes | 16-bit unsigned integer |
| `u32` | `uint32_t` | 4 bytes | 32-bit unsigned integer |
| `u64` | `uint64_t` | 8 bytes | 64-bit unsigned integer |
| `f32` | `float` | 4 bytes | 32-bit floating point |
| `f64` | `double` | 8 bytes | 64-bit floating point |
| `b8` | `bool` | 1 byte | Boolean (`true` / `false`) |
| `str` | `char*` | pointer | Null-terminated string |
| `ptr` | `void*` | pointer | Untyped (void) pointer |
| `ptr<T>` | `T*` | pointer | Typed pointer to `T` |
| `void` | `void` | — | No return value |
| `T[]` | `T*` | pointer | Dynamic array of `T` |
| `T[N]` | `T name[N]` | inline | Fixed-size inline array of `T` (value, not pointer) |
| `func(A, B) : R` | `R (*)(A, B)` | pointer | Function pointer type |

Numeric types are implicitly convertible between each other. Explicit conversion uses `cast()`.

```
var x : i32 = 42;
var y : f64 = cast(x, f64);
var z : i32 = cast(y, i32);       // truncates
var big : i64 = cast(x, i64);
var small : u8 = cast(x, u8);
```

### sizeof

```
var s : u64 = sizeof(i32);    // 4
var s2 : u64 = sizeof(f64);   // 8
var s3 : u64 = sizeof(ptr);   // 8
var s4 : u64 = sizeof(Vec2);  // sizeof the struct
```

---

### Operators

#### Arithmetic
```
a + b       // addition
a - b       // subtraction
a * b       // multiplication
a / b       // division
a % b       // modulo
-a          // unary negation
```

#### Comparison
```
a == b      // equal
a != b      // not equal
a < b       // less than
a > b       // greater than
a <= b      // less than or equal
a >= b      // greater than or equal
```

#### Logical
```
a && b      // logical AND
a || b      // logical OR
!a          // logical NOT
```

#### Bitwise
```
a & b       // bitwise AND
a | b       // bitwise OR
a ^ b       // bitwise XOR
~a          // bitwise NOT
a << n      // left shift
a >> n      // right shift
```

#### Assignment
```
x = 10;
x += 5;     x -= 3;     x *= 2;     x /= 4;     x %= 3;
x &= mask;  x |= flag;  x ^= bits;  x <<= n;    x >>= n;
```

#### Increment / Decrement
```
++x;        // prefix increment
--x;        // prefix decrement
x++;        // postfix increment
x--;        // postfix decrement
```

#### Operator Precedence

Standard C-like precedence. Multiplication/division bind tighter than addition/subtraction. Use parentheses to override.

```
var r : i32 = 1 + 2 * 3;      // 7, not 9
var s : i32 = (1 + 2) * 3;    // 9
```

---

### Functions

```
func add(a : i32, b : i32) : i32 {
    return a + b;
}

func greet(name : str) : void {
    println("Hello " + name);
}

func is_even(n : i32) : b8 {
    return n % 2 == 0;
}
```

Functions are declared with `func`, followed by the name, parameters in parentheses, a colon, and the return type. Parameters use `name : type` syntax.

#### Recursion

```
func factorial(n : i32) : i32 {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}
```

#### Nested Calls

```
var r : i32 = add(add(1, 2), add(3, 4));
```

---

### Control Flow

#### if / else if / else

```
if (x > 0) {
    println("positive");
} else if (x == 0) {
    println("zero");
} else {
    println("negative");
}
```

#### while

```
var i : i32 = 0;
while (i < 10) {
    println(to_str(i));
    i = i + 1;
}
```

#### for

```
for (var i : i32 = 0; i < 10; i++) {
    println(to_str(i));
}
```

The initializer declares a loop-scoped variable. The increment expression runs after each iteration.

```
for (var i : i32 = 10; i > 0; i--) {
    println(to_str(i));
}
```

#### break / continue

```
while (true) {
    if (done) {
        break;
    }
    if (skip) {
        continue;
    }
}
```

Both work in `while` and `for` loops. In nested loops, they affect the innermost loop.

#### switch

```
switch (value) {
    case 1:
        println("one");
    case 2:
        println("two");
    case 3:
        println("three");
    default:
        println("other");
}
```

Cases do **not** fall through by default (no `break` needed). Multiple case labels can share a body:

```
switch (x) {
    case 1:
    case 2:
    case 3:
        println("1, 2, or 3");
    default:
        println("other");
}
```

---

### Arrays

#### Declaration and Literals

```
var nums : i32[] = [1, 2, 3, 4, 5];
var empty : i32[] = [];
var floats : f64[] = [1.5, 2.5, 3.5];
var flags : b8[] = [true, false, true];
```

#### Indexing (0-based)

```
var first : i32 = nums[0];
nums[2] = 99;
var nested : i32 = nums[nums[0]];   // variable index
```

#### Array Builtins

```
nums.length()                  // returns i32 length
nums.push(42)                  // append element
var last : i32 = nums.pop()   // remove and return last element
free(nums)                     // deallocate array memory
```

#### Arrays of Any Type

```
var points : Vec2[] = [v1, v2, v3];
var strs : str[] = ["a", "b", "c"];
points[0].x = 10.0;
```

#### Fixed-Size Inline Arrays

The `type[N]` syntax declares a fixed-size C array embedded by value. These can appear as `@dataclass` fields (preserving exact C struct layout for FFI) or as local variables.

```
var buf : u8[64];         // stack-allocated 64-byte buffer
buf[0] = 255;
buf[63] = 1;

var ints : i32[4];
ints[0] = -1;
ints[3] = 999;
```

In a `@dataclass` this maps directly to the C struct field, making struct layouts ABI-compatible with C libraries:

```
@dataclass
class VkPhysicalDeviceProperties {
    var apiVersion        : u32;
    var driverVersion     : u32;
    var vendorID          : u32;
    var deviceID          : u32;
    var deviceType        : u32;
    var deviceName        : u8[256];          // char deviceName[256]
    var pipelineCacheUUID : u8[16];           // uint8_t pipelineCacheUUID[16]
}
```

Fixed-size arrays are **not** dynamic — they have no `.push()`, `.pop()`, or `.length()`. They are zero-initialised by default.

---

### Strings

Type: `str`

```
var s : str = "hello";
var s2 : str = `multiline or "quoted" string`;   // backtick strings
```

#### String Operations

```
var c : str = a + b;                 // concatenation
var eq : b8 = (a == b);             // equality
var neq : b8 = (a != b);           // inequality
```

#### String Methods

```
s.length()                          // returns i32 length
s.substring(start, length)          // returns substr
s.index_of("world")                // returns i32 index, -1 if not found
```

#### String Builtins

```
str_concat(a, b)                    // concatenate
str_compare(a, b)                   // returns 0 if equal (strcmp)
to_str(42)                          // i32 → str
to_str(3.14)                        // f64 → str
to_str(true)                        // b8 → str ("true"/"false")
```

---

### Pointers

#### Void Pointers

```
var p : ptr = null;
var x : i32 = 42;
var px : ptr = addr(x);    // take address, returns void*
if (px != null) { ... }
```

#### Typed Pointers

```
var x : i32 = 42;
var p : ptr<i32> = addr(x);       // ptr<i32> is int32_t*
var val : i32 = deref(p);          // read through pointer → 42
deref(p) = 100;                    // write through pointer, x is now 100
```

`addr(variable)` returns a `ptr<T>` where `T` is the type of the variable.  
`deref(pointer)` dereferences a typed pointer. It works as both an rvalue and lvalue.

#### Pointer Arithmetic

```
var p : ptr<i32> = addr(arr[0]);
var second : i32 = deref(p + 1);     // access next element
var elem : i32 = p[2];               // indexing on typed pointers
```

`ptr<T> + n` advances the pointer by `n * sizeof(T)` bytes (standard C pointer arithmetic).  
`ptr<T>[n]` indexes directly into the pointed-to memory.

#### Pointer Casting

```
var raw : ptr = cast(p, ptr);                // typed ptr → void ptr
var typed : ptr<i32> = cast(raw, ptr<i32>);  // void ptr → typed ptr
var fp : ptr<f32> = cast(ip, ptr<f32>);      // reinterpret pointer type
```

#### Pointer to Struct

```
var v : Vec2;
v.x = 1.0;
v.y = 2.0;
var pv : ptr<Vec2> = addr(v);
```

---

### Function Pointers

#### Declaration

```
var fp : func(i32, i32) : i32 = add;
```

The type syntax is `func(param_types) : return_type`. Assign a function name directly.

#### Calling

```
var result : i32 = fp(3, 4);    // call through function pointer
```

#### Reassignment

```
fp = multiply;                   // point to a different function
```

#### As Parameter

```
func apply(fn : func(i32, i32) : i32, x : i32, y : i32) : i32 {
    return fn(x, y);
}

var r : i32 = apply(add, 10, 20);
```

#### Null Function Pointer

```
var fp : func(i32) : void = null;
```

---

### Classes

```
class Point {
    var x : i32;
    var y : i32;

    func Point(px : i32, py : i32) : void {
        this.x = px;
        this.y = py;
    }

    func distance_sq() : i32 {
        return this.x * this.x + this.y * this.y;
    }

    func move(dx : i32, dy : i32) : void {
        this.x = this.x + dx;
        this.y = this.y + dy;
    }
}
```

- Constructor: same name as the class, returns `void`
- Member access: `this.field` or `self.field` (both work)
- Instantiation: `var p : Point = Point(10, 20);`
- Method calls: `p.distance_sq()`
- Field access: `p.x`

#### Field Defaults

```
class Counter {
    var count : i32 = 0;

    func Counter() : void {
        self.count = 0;
    }

    func increment() : void {
        self.count = self.count + 1;
    }

    func get() : i32 {
        return self.count;
    }
}
```

#### Arrays of Objects

```
var arr : Point[] = [Point(1, 2), Point(3, 4)];
var x : i32 = arr[0].x;
```

#### Destructors (RAII)

```
class Resource {
    var id : i32;

    func Resource(val : i32) : void {
        this.id = val;
    }

    func ~Resource() : void {
        println("cleaning up resource " + to_str(this.id));
    }
}
```

The destructor `~ClassName()` is called automatically when the object goes out of scope. Destructors of derived classes call parent destructors automatically.

### Inheritance

```
class Shape {
    var x : i32;
    var y : i32;

    func Shape(px : i32, py : i32) : void {
        this.x = px;
        this.y = py;
    }

    func area() : i32 {
        return 0;
    }
}

class Circle : Shape {
    var radius : i32;

    func Circle(px : i32, py : i32, r : i32) : void {
        this.x = px;
        this.y = py;
        this.radius = r;
    }

    func area() : i32 {
        return 3 * this.radius * this.radius;
    }
}
```

- Single inheritance via `: ParentClass`
- Child inherits all parent fields and methods
- Methods can be overridden
- Child can add new fields and methods

### Interfaces

```
interface Printable {
    func describe() : str;
}

interface HasArea {
    func area() : i32;
}

class Box implements Printable, HasArea {
    var w : i32;
    var h : i32;

    func Box(width : i32, height : i32) : void {
        this.w = width;
        this.h = height;
    }

    func describe() : str {
        return "box";
    }

    func area() : i32 {
        return this.w * this.h;
    }
}
```

A class can implement multiple interfaces (comma-separated). All interface methods must be provided.

---

### Dataclass (Structs)

```
@dataclass
class Vec2 {
    var x : f64;
    var y : f64;
}
```

The `@dataclass` decorator creates a stack-allocated value type (C struct). No constructor needed.

```
var v : Vec2;
v.x = 1.0;
v.y = 2.0;

var copy : Vec2 = v;     // value copy — modifying copy does not affect v
copy.x = 999.0;          // v.x is still 1.0
```

#### Nested Dataclass

```
@dataclass
class Color {
    var r : u8;
    var g : u8;
    var b : u8;
    var a : u8;
}

@dataclass
class Vertex {
    var pos : Vec2;
    var col : Color;
}
```

#### Dataclass in Functions

```
func make_vec(x : f64, y : f64) : Vec2 {
    var v : Vec2;
    v.x = x;
    v.y = y;
    return v;
}

func add_vec(a : Vec2, b : Vec2) : Vec2 {
    var result : Vec2;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}
```

#### sizeof on Dataclass

```
var s : u64 = sizeof(Vec2);    // 16 (two f64 fields)
```

---

### Enums

```
enum Color {
    RED,
    GREEN,
    BLUE
}
```

Values auto-increment from 0. Explicit values supported:

```
enum Status {
    OK = 0,
    ERROR = 1,
    PENDING = 100,
    DONE
}
```

`DONE` equals 101 (auto-increments from previous).

#### Usage

```
var c : i32 = Color.RED;          // 0
var s : i32 = Status.PENDING;     // 100

if (c == Color.GREEN) { ... }

switch (s) {
    case 0:
        println("OK");
    case 100:
        println("PENDING");
}
```

Enum values are `i32`.

---

### Unions

```
union Data {
    i : i32;
    f : f32;
    b : b8;
}
```

Shared memory — writing one field overwrites others.

```
var d : Data;
d.i = 42;
println(to_str(d.i));    // 42

d.f = 3.14;              // overwrites d.i
```

Unions can contain dataclass types:

```
union Geometry {
    v2 : Vec2;
    v3 : Vec3;
}

var g : Geometry;
g.v2.x = 1.0;
g.v2.y = 2.0;
```

---

### Defer

```
func example() : void {
    defer println("cleanup 1");
    defer println("cleanup 2");
    println("working");
}
// Output: working, cleanup 2, cleanup 1
```

`defer` schedules a statement to execute at scope exit, in LIFO (last-in-first-out) order. Each block scope (function, if, for, while) has its own defer stack.

```
for (var i : i32 = 0; i < 3; i++) {
    defer println("deferred " + to_str(i));
    println("loop " + to_str(i));
}
// Each iteration: "loop N" then "deferred N"
```

---

### Try / Catch / Throw

```
func risky(should_throw : b8) : i32 {
    if (should_throw) {
        throw "something went wrong";
    }
    return 42;
}

func main() : i32 {
    var result : i32 = 0;
    try {
        result = risky(true);
    } catch (e : str) {
        println("caught: " + e);
    }
    return 0;
}
```

- `throw` takes a string expression
- `catch` binds the thrown value to a typed variable
- Nested try/catch is supported
- You can rethrow from within a catch block

---

### Signals

Signals are typed message queues for inter-process communication.

```
signal my_signal : i32;

my_signal.emit(42);                      // send a value
var val : i32 = my_signal.recv();        // receive a value (FIFO)
```

#### Signal Arrays

```
signal channels[4] : i32;

channels[0].emit(10);
channels[1].emit(20);
var v : i32 = channels[0].recv();
```

#### Signals with Complex Types

```
signal point_signal : Point;
signal data_signal : i32[];

var pts : Point = Point(1, 2);
point_signal.emit(pts);
var p : Point = point_signal.recv();

var arr : i32[] = [1, 2, 3];
data_signal.emit(arr);
var received : i32[] = data_signal.recv();
```

---

### Events and Processes

Events trigger one or more process handlers. Processes run concurrently (pthreads).

```
event on_start;
signal done : i32;
var result : i32 = 0;

@on_start process handler {
    result = 42;
    done.emit(1);
}

func main() : i32 {
    on_start.execute();       // triggers all bound processes
    done.recv();              // wait for completion
    println(to_str(result));  // 42
    return 0;
}
```

Multiple processes can be bound to the same event:

```
@my_event process worker1 {
    result = result + 10;
}

@my_event process worker2 {
    result = result + 20;
}

my_event.execute();    // both run
```

---

### Extern Functions (C Interop)

```
extern func abs(x : i32) : i32;
extern func malloc(size : u64) : ptr;
extern func free(p : ptr) : void;
extern func memcpy(dst : ptr, src : ptr, n : u64) : ptr;
```

Extern functions declare C functions that are linked at compile time. They are called like regular functions.

```
var result : i32 = abs(-42);
```

### Link Directive

```
link "-lvulkan";
link "-lglfw3";
link "-ldl";
```

The `link` directive passes flags directly to GCC at link time. Use it to link external libraries.

### Full Extern Example

```
link "-lm";
extern func sin(x : f64) : f64;
extern func cos(x : f64) : f64;

func main() : i32 {
    var angle : f64 = 3.14159;
    var s : f64 = sin(angle);
    var c : f64 = cos(angle);
    return 0;
}
```

---

### Imports / Modules

```
import math;                          // import entire module
from utils import helper1, helper2;   // import specific names
from graphics import *;               // import all from module
```

Modules are resolved relative to the current file path.

---

### Comments

```
/* block comment */

/*
   multi-line
   block comment
*/

func /* inline */ add(a : i32, b : i32) : i32 { ... }
```

Block comments using `/* ... */`. These can be single-line, multi-line, or inline.

---

### Conditional Compilation

Use `@if(DEFINE)` / `@else` blocks to include or exclude top-level declarations at compile time. Define symbols on the command line with `-D` or `-DNAME`.

```
@if(MACOS) {
    link "-lMoltenVK";
    const PORTABILITY_EXT : str = "VK_KHR_portability_enumeration";
} @else {
    const PORTABILITY_EXT : str = "";
}

@if(DEBUG) {
    func log(msg : str) : void { println("[DEBUG] " + msg); }
} @else {
    func log(msg : str) : void { }
}
```

Compile:

```bash
tick src.tick -DMACOS -DDEBUG -o out
tick src.tick -D MACOS -D DEBUG -o out   # space form also accepted
```

- Both the `@if` and `@else` blocks may contain any number of top-level declarations (functions, consts, classes, extern declarations, link directives, etc.).
- `@else` is optional.
- Nesting is not supported.
- Only a single identifier is tested — there are no `&&`/`||` combinator expressions.

---

### Builtin Functions Reference

#### I/O

| Function | Signature | Description |
|----------|-----------|-------------|
| `print(...)` | `any → void` | Print without newline |
| `println(...)` | `any → void` | Print with newline |
| `input()` | `→ str` | Read line from stdin |
| `input(prompt)` | `str → str` | Read line with prompt |

#### Math

| Function | Signature | Description |
|----------|-----------|-------------|
| `sqrt(x)` | `f64 → f64` | Square root |
| `pow(x, y)` | `f64, f64 → f64` | Power |
| `sin(x)` | `f64 → f64` | Sine |
| `cos(x)` | `f64 → f64` | Cosine |
| `tan(x)` | `f64 → f64` | Tangent |
| `floor(x)` | `f64 → f64` | Floor |
| `ceil(x)` | `f64 → f64` | Ceiling |
| `round(x)` | `f64 → f64` | Round |
| `log(x)` | `f64 → f64` | Natural log |
| `log2(x)` | `f64 → f64` | Log base 2 |
| `log10(x)` | `f64 → f64` | Log base 10 |
| `fmin(a, b)` | `f64, f64 → f64` | Minimum |
| `fmax(a, b)` | `f64, f64 → f64` | Maximum |
| `abs(x)` | `i32 → i32` | Absolute value |

#### Conversion

| Function | Signature | Description |
|----------|-----------|-------------|
| `to_str(x)` | `any → str` | Convert to string |
| `parse(s)` | `str → inferred` | Parse string to number/bool |
| `cast(x, T)` | `any, type → T` | Type cast |
| `sizeof(T)` | `type → u64` | Size of type in bytes |

#### Memory & Pointers

| Function | Signature | Description |
|----------|-----------|-------------|
| `addr(x)` | `T → ptr<T>` | Address of variable |
| `deref(p)` | `ptr<T> → T` | Dereference typed pointer (rvalue and lvalue) |
| `free(x)` | `any → void` | Free GC-managed memory |
| `malloc(n)` | `u64 → ptr` | Allocate `n` bytes (raw, unmanaged) |
| `memset(dst, c, n)` | `ptr, i32, u64 → ptr` | Fill memory with byte value |
| `memcpy(dst, src, n)` | `ptr, ptr, u64 → ptr` | Copy `n` bytes (no overlap) |
| `memmove(dst, src, n)` | `ptr, ptr, u64 → ptr` | Copy `n` bytes (overlap-safe) |
| `memcmp(a, b, n)` | `ptr, ptr, u64 → i32` | Compare `n` bytes (0 = equal) |
| `gc_collect()` | `→ void` | Trigger garbage collection |
| `gc_cleanup()` | `→ void` | Final GC cleanup |

#### Strings

| Function | Signature | Description |
|----------|-----------|-------------|
| `str_concat(a, b)` | `str, str → str` | Concatenate strings |
| `str_compare(a, b)` | `str, str → i32` | Compare (0 = equal) |
| `strlen(s)` | `str → i32` | String length |

#### File I/O

| Function | Signature | Description |
|----------|-----------|-------------|
| `file_open(path, mode)` | `str, str → ptr` | Open file |
| `file_read(handle)` | `ptr → str` | Read file contents |
| `file_write(handle, data)` | `ptr, str → void` | Write to file |
| `file_close(handle)` | `ptr → void` | Close file |
| `file_exists(path)` | `str → b8` | Check if file exists |

---

### Complete Example: Vulkan-style FFI Pattern

```
link "-lvulkan";

@dataclass
class VkApplicationInfo {
    var sType : i32;
    var pNext : ptr;
    var pApplicationName : str;
    var applicationVersion : u32;
    var pEngineName : str;
    var engineVersion : u32;
    var apiVersion : u32;
}

@dataclass
class VkInstanceCreateInfo {
    var sType : i32;
    var pNext : ptr;
    var flags : u32;
    var pApplicationInfo : ptr<VkApplicationInfo>;
    var enabledLayerCount : u32;
    var ppEnabledLayerNames : ptr;
    var enabledExtensionCount : u32;
    var ppEnabledExtensionNames : ptr;
}

extern func vkCreateInstance(
    pCreateInfo : ptr<VkInstanceCreateInfo>,
    pAllocator : ptr,
    pInstance : ptr<ptr>
) : i32;

extern func vkDestroyInstance(instance : ptr, pAllocator : ptr) : void;

func main() : i32 {
    var app_info : VkApplicationInfo;
    app_info.sType = 0;
    app_info.pNext = null;
    app_info.pApplicationName = "Tick App";
    app_info.applicationVersion = 1;
    app_info.pEngineName = "Tick Engine";
    app_info.engineVersion = 1;
    app_info.apiVersion = 4194304;

    var create_info : VkInstanceCreateInfo;
    create_info.sType = 1;
    create_info.pNext = null;
    create_info.flags = 0;
    create_info.pApplicationInfo = addr(app_info);
    create_info.enabledLayerCount = 0;
    create_info.ppEnabledLayerNames = null;
    create_info.enabledExtensionCount = 0;
    create_info.ppEnabledExtensionNames = null;

    var instance : ptr = null;
    var result : i32 = vkCreateInstance(addr(create_info), null, addr(instance));

    if (result == 0) {
        println("Vulkan instance created");
        vkDestroyInstance(instance, null);
    } else {
        println("Failed to create instance: " + to_str(result));
    }

    return 0;
}
```

---

### Grammar Summary

```
program        → (import | link | global_var | const | extern_func |
                   class | dataclass | enum | union | interface |
                   event | signal | process | function |
                   conditional_compile)*

import         → "import" IDENTIFIER ";"
               | "from" IDENTIFIER "import" ("*" | IDENTIFIER ("," IDENTIFIER)*) ";"

link           → "link" STRING ";"

global_var     → "var" IDENTIFIER ":" type ("=" expression)? ";"
const          → "const" IDENTIFIER ":" type "=" expression ";"

function       → "func" IDENTIFIER "(" parameters? ")" ":" type block
parameters     → parameter ("," parameter)*
parameter      → IDENTIFIER ":" type

extern_func    → "extern" "func" IDENTIFIER "(" parameters? ")" ":" type ";"

class          → ("@dataclass")? "class" IDENTIFIER (":" IDENTIFIER)?
                 ("implements" IDENTIFIER ("," IDENTIFIER)*)? "{" class_body "}"
class_body     → (var_decl | method)*
method         → "func" ("~")? IDENTIFIER "(" parameters? ")" ":" type block

interface      → "interface" IDENTIFIER "{" interface_method* "}"
interface_method → "func" IDENTIFIER "(" parameters? ")" ":" type ";"

enum           → "enum" IDENTIFIER "{" enum_value ("," enum_value)* "}"
enum_value     → IDENTIFIER ("=" INTEGER)?

union          → "union" IDENTIFIER "{" union_field* "}"
union_field    → IDENTIFIER ":" type ";"

signal         → "signal" IDENTIFIER ("[" INTEGER "]")? ":" type ";"
event          → "event" IDENTIFIER ";"
process        → "@" IDENTIFIER "process" IDENTIFIER block

conditional_compile
               → "@" "if" "(" IDENTIFIER ")" "{" program_decl* "}"
                   ("@" "else" "{" program_decl* "}")?

type           → "i8" | "i16" | "i32" | "i64"
               | "u8" | "u16" | "u32" | "u64"
               | "f32" | "f64" | "b8" | "str"
               | "void" | "ptr" | "ptr" "<" type ">"
               | "func" "(" type_list? ")" ":" type
               | IDENTIFIER
               | type "[" "]"
               | type "[" INTEGER "]"

block          → "{" statement* "}"

statement      → var_decl | const_decl | expression_stmt
               | if_stmt | while_stmt | for_stmt
               | switch_stmt | return_stmt | break_stmt
               | continue_stmt | defer_stmt | try_catch
               | throw_stmt | block

var_decl       → "var" IDENTIFIER ":" type ("=" expression)? ";"
if_stmt        → "if" "(" expression ")" block ("else" (if_stmt | block))?
while_stmt     → "while" "(" expression ")" block
for_stmt       → "for" "(" (var_decl | expression) ";" expression ";" expression ")" block
switch_stmt    → "switch" "(" expression ")" "{" case_clause* "}"
case_clause    → ("case" expression ":" | "default" ":") statement*
return_stmt    → "return" expression? ";"
break_stmt     → "break" ";"
continue_stmt  → "continue" ";"
defer_stmt     → "defer" statement
try_catch      → "try" block "catch" "(" IDENTIFIER ":" type ")" block
throw_stmt     → "throw" expression ";"

expression     → assignment
assignment     → (call "=")? ternary
               | (call compound_op)? ternary
ternary        → logical_or
logical_or     → logical_and ("||" logical_and)*
logical_and    → bitwise_or ("&&" bitwise_or)*
bitwise_or     → bitwise_xor ("|" bitwise_xor)*
bitwise_xor    → bitwise_and ("^" bitwise_and)*
bitwise_and    → equality ("&" equality)*
equality       → comparison (("==" | "!=") comparison)*
comparison     → shift (("<" | ">" | "<=" | ">=") shift)*
shift          → addition (("<<" | ">>") addition)*
addition       → multiplication (("+" | "-") multiplication)*
multiplication → unary (("*" | "/" | "%") unary)*
unary          → ("!" | "-" | "~" | "++" | "--") unary | postfix
postfix        → primary ("++" | "--" | "." IDENTIFIER | "[" expression "]"
                          | "(" arguments? ")")*
primary        → INTEGER | FLOAT | DOUBLE | STRING | "true" | "false" | "null"
               | IDENTIFIER | "this" | "self"
               | "(" expression ")"
               | "[" (expression ("," expression)*)? "]"
               | "cast" "(" expression "," type ")"
               | "sizeof" "(" type ")"
               | IDENTIFIER "(" arguments? ")"
```

---

### Compilation Model

Tick transpiles to C, then compiles with GCC:

1. **Lexer** tokenizes source into tokens
2. **Parser** builds an AST
3. **Semantic Analyzer** performs type checking and validation
4. **C Code Generator** produces a `.c` file
5. **GCC** compiles the C file with the tick runtime (`tick_runtime.c`) using `-O2 -pthread -lm`

The tick runtime provides: GC (`tick_gc_*`), signal queues (`TickSignal`), string operations (`tick_str_*`), file I/O (`tick_file_*`), and event dispatch (`TickEvent`).

Classes (non-dataclass) are heap-allocated and GC-managed (`void*` pointers in C).  
Dataclasses are stack-allocated structs (value types in C).

### File Extension

`.tick`
