# Tick

Tick is a statically-typed, compiled systems language that transpiles to C. Its
guiding principle: **the programmer reasons about what the program does, never about
who frees what** — while performance stays deterministic and predictable.

- **Values by default.** Everything is a value unless you explicitly write `ref` or
  `shared`. There is no hidden aliasing and no hidden heap allocation.
- **Automatic, deterministic memory.** Owned heap data is reclaimed at compiler-chosen
  points. No `free`, no garbage collector, no pauses.
- **Cost is readable.** A type tells you its cost: a value, a borrow, or a shared
  (reference-counted) handle.
- **Validation is a layer, not a tax.** Bounds/null checks run during development and
  compile out entirely in release builds (the Vulkan validation-layer model).

The full language design and rationale live in [DESIGN_V2.md](DESIGN_V2.md).

## Building the compiler

```bash
cmake -B build
cmake --build build
```

Produces `./build/tick`.

## Usage

```bash
tick prog.tick                 # default: bounds + null checks
tick prog.tick --release       # strip all checks, maximum speed
tick prog.tick --validate      # full validation layer
tick prog.tick -o prog         # choose output name
tick prog.tick --emit-c        # print generated C and exit
tick prog.tick --keep-c        # keep the generated C next to the binary
```

Pipeline: **Tick → Lexer → Parser → Checker (types + ownership) → C codegen → cc**.

---

## Language

### Bindings

Immutability is the default; mutation is explicit.

```
let x = 10        // immutable
var y = 20        // mutable
y = 30            // ok
// x = 11         // error: cannot assign to a `let` binding
```

Types are inferred, or written after a colon:

```
let n : i32 = 42
var s : str = "hi"
```

### Types

| Type        | Meaning                              | Cost              |
|-------------|--------------------------------------|-------------------|
| `i8`..`i64`, `u8`..`u64` | sized integers          | value             |
| `f32`, `f64`| floats                               | value             |
| `bool`      | `true` / `false`                     | value             |
| `str`       | immutable owned string               | heap (automatic)  |
| `T[]`       | dynamic array of values              | heap (automatic)  |
| `T[N]`      | fixed inline array                   | value             |
| `struct`    | user value type                      | value             |
| `ref T`     | borrow (non-owning reference)        | a pointer         |
| `shared T`  | shared, reference-counted ownership  | refcount          |
| `weak T`    | non-owning handle to a `shared T`    | a pointer         |
| `ptr`       | raw pointer (only inside `unsafe`)   | a pointer         |

### Functions

```
func add(a : i32, b : i32) : i32 {
    return a + b
}
```

References are explicit on both sides — a reader sees aliasing at the call site:

```
func grow(ref var a : i32[]) { a.push(1) }   // mutable borrow
func sum(ref a : i32[]) : i32 { ... }          // read-only borrow

var nums : i32[] = [1, 2, 3]
grow(ref var nums)
let total = sum(ref nums)
```

### Structs

A `struct` is a pure value type — fields only, no inheritance.

```
struct Vec2 { x : f64, y : f64 }

var v = Vec2 { x = 3.0, y = 4.0 }
var w = v          // value copy — independent
w.x = 99.0
// v.x is still 3.0
```

Methods are added in `impl` blocks; the receiver `self` is explicit:

```
impl Vec2 {
    func length_sq(self) : f64 { return self.x * self.x + self.y * self.y }
    func translate(ref var self, dx : f64, dy : f64) {
        self.x = self.x + dx
        self.y = self.y + dy
    }
}
```

### Interfaces

Interfaces are compile-time contracts — no vtable, no runtime cost. The compiler
verifies a type provides every required method.

```
interface Shape { func area(self) : f64 }

struct Circle { r : f64 }
impl Circle { func area(self) : f64 { return 3.14159 * self.r * self.r } }
impl Shape for Circle {}     // verified: Circle has area(self) : f64
```

### Enums and match

```
enum Dir { North, East, South, West }
enum Status { Ok = 0, Warn = 10, Fail = 20 }

match d {
    North => turn(),
    East  => { step(); turn() },
    _     => stop(),
}
```

`match` over an enum is exhaustive — the checker rejects missing variants unless a
`_` wildcard is present.

### Arrays

```
var a : i32[] = [1, 2, 3]
a.push(4)
let first = a[0]        // bounds-checked in dev/validate, raw in release
let n = a.len()
let last = a.pop()

for x in a { use(x) }        // iterate by value
for ref x in a { mutate(x) } // iterate by borrow
```

A plain binding `var b = a` is a deep value copy — `a` and `b` are independent.

### Strings

```
let s = "hello " + name      // concatenation (owned result, auto-freed)
let eq = (a == b)            // equality
let n = s.len()
let order = str_order(a, b)  // <0, 0, >0 for sorting
let text = to_str(42)        // number/bool -> str
```

### Shared and weak

`shared` is the only place a runtime-managed lifetime appears, and it is always
visible in the type. Copying a `shared` handle bumps the refcount; it is released
automatically when the last handle goes out of scope.

```
struct Node { value : i32 }

var root : shared Node = shared Node { value = 0 }
var also = root              // shares ownership (refcount 2)
var back : weak Node = weak root   // breaks reference cycles
```

### Control flow

```
if cond { ... } else if cond { ... } else { ... }
while cond { ... }
for i in 0..n { ... }
break    continue    return expr
defer stmt            // runs at scope exit, LIFO (effectful teardown)
```

`defer` is for effectful cleanup (logging, closing handles) — memory is automatic.

### Concurrency: signals, events, processes

A value sent over a signal is moved into the channel; the receiver owns it.

```
signal done : i32;
event start;

process worker on start {
    done.emit(compute())
}

func main() : i32 {
    start.fire()           // run bound processes
    let r = done.recv()    // receive (FIFO); receiver owns r
    return 0
}
```

### C interop

Raw pointers, `malloc`/`free`, and pointer arithmetic exist only inside `unsafe`.

```
extern func sin(x : f64) : f64;
link "-lm";

unsafe {
    let p : ptr = malloc(64)
    free(p)
}
```

---

## Build modes & the validation layer

| Mode          | Checks inserted                          | Use            |
|---------------|------------------------------------------|----------------|
| `tick`        | bounds + null                            | daily work     |
| `--validate`  | + overflow, use-after-move, weak-upgrade | development    |
| `--release`   | none — checks compiled out               | shipping       |

In every mode the optimizer elides checks it can prove unnecessary. Release builds
are safe for the statically-proven subset and pay nothing for validation.

---

## Performance guarantees

1. **No hidden allocation.** Heap touches only at `shared`, array growth, or `unsafe`.
2. **No hidden free / no GC.** Frees are compiler-inserted at deterministic points.
3. **Predictable layout.** `struct` = C struct, `ref` = pointer, `shared` =
   `{refcount, payload}`.
4. **Zero-cost validation in release.** All checks compile out.

---

## Tests

```bash
bash tests/run_test_suite.sh
```

Each test is compiled and run in default and `--release` modes, and verified
memory-clean under AddressSanitizer + UBSan.

## File extension

`.tick`
