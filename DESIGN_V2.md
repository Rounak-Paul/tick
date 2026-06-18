# Tick 2.0 — Language Design

## Vision

The programmer reasons about *what the program does*, never about *who frees what*.
Performance is deterministic and predictable. Memory cost is visible in the shape of
the code, not hidden. References are intentional and rare; values are the default.
Safety is a development-time validation layer, not a runtime tax in production.

Three non-negotiable guarantees:

1. **No GC, no pauses.** Memory is reclaimed at deterministic, compiler-chosen points.
2. **No memory bookkeeping in normal code.** No `free`, no lifetimes, no `&`.
3. **Cost is readable.** A type tells you its cost: value, borrow, or shared.

Target domain: systems / real-time (games, audio, embedded). Latency predictability
beats peak convenience.

---

## 1. The mental model

Everything is a **value** unless the code explicitly says otherwise. Aliasing never
happens implicitly — it is always spelled out with `ref` or `shared`.

| Form       | Meaning                                   | Runtime cost     | C lowering              |
|------------|-------------------------------------------|------------------|-------------------------|
| `T`        | Owned value (stack / inline)              | none             | `T` by value            |
| `ref T`    | Borrow — temporary, non-owning reference  | none (a pointer) | `T*` (lifetime-checked) |
| `shared T` | Shared ownership, reference counted       | refcount inc/dec | `{ rc; T }*`            |
| `weak T`   | Non-owning ref to a `shared T` (no cycle) | guard on upgrade | `{ rc; T }*` (no rc)    |
| `T[]`      | Dynamic array of values                   | one heap alloc   | `TickArray`             |
| `T[N]`     | Fixed inline array                        | none             | `T[N]`                  |

The single rule that makes this intuitive:

> **`ref` is the only way to not-copy. If you don't write `ref` (or `shared`),
> it is a value.** There is no hidden aliasing anywhere in the language.

### Why this is simpler than Rust

- One borrow form: `ref`. No `&` / `&mut` / lifetime parameters in source.
- Assignment is **always** a copy of a value (big values are cheap — see §3 move
  elision). No move-vs-copy ambiguity to learn.
- Borrow checking still runs, but the common path (value in, value out) never
  touches it.

---

## 2. Bindings and mutability

`var` is the single binding keyword. Mutability is the default; immutability is a
type qualifier. This is exactly C's model — no `let`/`val` keyword clutter.

```
var x = 10                  // mutable
var pi : const f64 = 3.14   // immutable — `const` qualifies the type, not the decl
pi = 3.0                    // error: cannot assign to const binding
```

`const` composes with any type:

```
var root : const shared Node = Node(...)   // immutable handle; node fields follow their own mutability
var buf  : const i32[] = [1, 2, 3]        // immutable array binding
```

`ref` is the single borrow form — it means pass by pointer, no copy. Whether the
function mutates through that pointer is the function's own business, same as C.
The call site is always clean — just pass the value.

```
func grow(ref a : i32[]) { a.push(1) }  // borrows a, mutates through it
func sum(ref a : i32[]) : i32 { ... }   // borrows a, read-only

var nums : i32[] = [1, 2, 3]
grow(nums)             // no annotation needed — compiler passes &nums
var total = sum(nums)
```

Method receivers follow the same model:

```
func area(self) : f64 { ... }        // value copy of self
func inspect(ref self) : str { ... } // borrow self — may or may not mutate
```

---

## 3. Ownership inference and move elision

The programmer writes pure value code. The compiler infers ownership and reclaims
memory at deterministic points.

```
func build() : Mesh {
    var m = Mesh.empty()
    fill(m)                // mut borrow, mutate in place — no copy
    return m               // last use of `m` -> MOVE out, not copy
}

func main() : i32 {
    var mesh = build()     // ownership transferred in; freed at end of main
    return 0
}
```

What the compiler does, invisibly:

- Constructs `m` in place.
- Recognizes `return m` as the **last use** of `m` → lowers to a move (no copy, no
  free of `m`).
- Inserts exactly one `free` for `mesh` at the end of its scope.

### Analysis

- **Last-use / liveness**: for each owned value, find the program point after which
  it is never used. Insert reclamation there (scope exit, or earlier on last use
  before an early `return`/`break`/`continue`).
- **Move on last-use pass**: a value passed by value at its last use is moved, not
  copied. Earlier uses copy.
- **Borrow checking**: a `ref` may not outlive the value it borrows; a value may not
  be moved while a live `ref` to it exists; no two `mut` borrows of the same
  value are simultaneously live.

### The keystone rule (no annotations + always-compiles + no GC)

When ownership cannot be statically proven (e.g. a value escapes into a long-lived
graph through paths the analyzer can't linearize), the compiler **does not reject and
does not require annotations**. It **auto-promotes that value to `shared`** (reference
counted) and emits a validation-layer note:

```
note[perf]: `node` auto-promoted to `shared` (ownership not statically provable)
  --> scene.tick:42:9
  consider making it `shared Node` explicitly, or restructuring to keep it owned
```

This is the Vulkan-style perf warning: production still works and stays safe; an
expert optimizing a hot path sees exactly where an invisible refcount was inserted
and can act on it. Correctness never depends on the programmer reacting to it.

---

## 4. Shared and weak

`shared` is the **only** place a runtime-managed lifetime (refcount) appears, and it
is always visible in the type.

```
struct Node {
    value : i32
    children : shared Node []
    parent : weak Node
}

var root : shared Node = Node(value = 0, children = [], parent = none)
var child = root            // shares root (refcount 2) — visible: type is `shared`
child.parent = weak root    // back-edge, no cycle leak
```

- `shared T`: heap object `{ strong_rc, weak_rc, T }`. Copy increments strong rc;
  drop decrements; freed at zero.
- `weak T`: holds the object without owning it. No refcount bump; the pointed-to
  object is freed when all `shared` handles drop regardless of live `weak` handles.

There is no other shared/aliased state in the language. If a type is not `shared`,
it cannot be long-term aliased.

---

## 5. Memory model

The type tells you where data lives. There are no surprises.

| Type          | Handle (variable) | Backing data  | Freed by              |
|---------------|-------------------|---------------|-----------------------|
| `i32`, `f64`  | stack             | —             | scope exit (automatic)|
| `bool`        | stack             | —             | scope exit (automatic)|
| `struct T`    | stack (inline)    | —             | scope exit (automatic)|
| `T[N]`        | stack (inline)    | —             | scope exit (automatic)|
| `ref T`       | stack (pointer)   | wherever T is | never (non-owning)    |
| `weak T`      | stack (pointer)   | wherever T is | never (non-owning)    |
| `str`         | stack (handle)    | heap (buffer) | compiler-inserted free|
| `T[]`         | stack (handle)    | heap (buffer) | compiler-inserted free|
| `shared T`    | stack (handle)    | heap (box+T)  | refcount → 0          |

**The rule:** you only think about stack. The compiler handles heap. The type tells
you whether heap is involved at all — if you don't see `str`, `T[]`, or `shared`,
nothing is on the heap.

---

## 6. Safety as a validation layer (build modes)

Modeled on Vulkan validation layers: exhaustive during development, zero cost in
production.

| Command                 | Checks inserted                                                              | Use            |
|-------------------------|-----------------------------------------------------------------------------|----------------|
| `tick build --validate` | bounds, integer overflow, use-after-move, weak-upgrade, leak audit          | development    |
| `tick build`            | bounds                                                                       | daily iteration|
| `tick build --release`  | **none** — only statically proven-safe code; checks elided                  | shipping       |

- In every mode the optimizer **elides checks it can prove unnecessary** (e.g.
  `for i in 0..a.len { a[i] }` needs no bounds check). Validation builds stay usable.
- `--release` is safe for the proven subset because the ownership model is statically
  sound; the only code allowed to bypass proofs is inside an explicit `unsafe` block.

```
var x = arr[i]          // validate/dev: bounds-checked; release: raw if proven
```

---

## 7. Unsafe and FFI

Raw pointers exist only for FFI and hand-tuned hot paths, confined to `unsafe`.

```
unsafe {
    var p : ptr = malloc(64)
    write(p, 0, 255)
    free(p)
}

extern func vkCreateInstance(info : ref VkInstanceCreateInfo, out : ptr) : i32
link "-lvulkan"
```

Outside `unsafe`, raw `ptr`, pointer arithmetic, `malloc`/`free` do not exist. The
safe language has no way to produce a dangling pointer or a double free.

---

## 8. Types

### Primitives
`i8 i16 i32 i64  u8 u16 u32 u64  f32 f64  bool  str  void`

`str` is an immutable, length-prefixed UTF-8 value (owned). Concatenation produces a
new value; the compiler frees temporaries by ownership inference (no manual `free`).

### Composite
- `struct` — the only user value type. Fields, no inheritance.
- `interface` — a contract (set of method signatures).
- `enum` — named integer constants, exactly like C enums.

### No classes, no inheritance

`struct` + `impl` + `interface` + composition replaces OOP. There is no base/derived,
no implicit `this` heap object, no RAII-via-destructor (reclamation is by ownership
inference, not destructors — though a type may define `drop` for custom teardown,
called automatically at reclamation).

```
struct Circle { r : f64 }

impl Circle {
    func area(self) : f64 { return 3.14159 * self.r * self.r }
}

interface Shape {
    func area(self) : f64
}

impl Shape for Circle {}        // Circle already has area(self):f64 -> satisfies Shape
```

`self` is the receiver, always explicit in the signature. `self` is a value;
`ref self` borrows read-only; `mut self` borrows mutably.

---

## 9. Methods, interfaces, dispatch

- `impl T { ... }` adds methods to `T`.
- `impl I for T {}` declares that `T` satisfies interface `I` (compiler verifies the
  methods exist). **Static, zero-cost.**
- Dynamic dispatch is opt-in and visible: `dyn I` is a fat pointer (data + vtable).
  Only `dyn I` costs a vtable indirection; plain interface use is monomorphized.

```
func describe(s : dyn Shape) : f64 { return s.area() }   // dynamic, explicit `dyn`
func area_of(c : Circle) : f64 { return c.area() }        // static, inlined
```

---

## 10. Control flow

```
if cond { ... } else if cond { ... } else { ... }
while cond { ... }
for i in 0..n { ... }            // half-open range
for x in items { ... }            // iterate values (copy)
for ref x in items { ... }        // borrow each element
match value { pattern => expr, ... }
break    continue    return expr
defer stmt                        // runs at scope exit, LIFO (for non-memory teardown)
```

`defer` is for *effectful* teardown (closing a handle, logging) — not for memory,
which is automatic. Parentheses around conditions are optional (`if x > 0 {`).

---

## 11. Concurrency: signals, events, processes

A value sent over a signal is **moved** into the channel (ownership transfers); the
receiver owns it. No shared mutable state without `shared`.

```
signal done : i32
event on_start

process worker on on_start {
    done.emit(compute())          // moves the value into the channel
}

func main() : i32 {
    on_start.fire()
    var r = done.recv()           // receiver now owns r
    return 0
}
```

---

## 12. Modules

```
import math                       // whole module, namespaced: math.sqrt(x)
from utils import helper          // selective
pub func exported() { ... }       // only `pub` items are importable
```

`pub` gives real encapsulation; non-`pub` items are module-private.

---

## 13. Performance guarantees (the contract)

1. **No hidden allocation.** Heap touches only at: `shared`, `T[]` growth, explicit
   `unsafe` malloc. `var p = Point()` never heap-allocates.
2. **No hidden free / no GC.** Frees are compiler-inserted at deterministic points;
   each is O(1).
3. **No hidden copy in release.** Last-use value passes lower to moves.
4. **Predictable layout.** `struct` = C struct. `ref` = pointer. `shared` =
   `{rc, payload}`. `dyn I` = `{data, vtable}`. No vtable unless `dyn`.
5. **Readable cost.** The type is the cost model.
6. **Zero-cost validation in release.** All checks compile out.

---

## 14. Compilation pipeline

```
source
  -> Lexer        tokens
  -> Parser       AST
  -> Resolver     names, modules, imports
  -> TypeChecker  types, interface satisfaction, match exhaustiveness
  -> Ownership    borrow check, liveness, move elision, auto-shared promotion
  -> CodeGen      C with validation-layer checks gated by build mode
  -> cc           gcc/clang -O2 (-pthread -lm), checks compiled out in --release
```

Each stage has one responsibility; no stage reaches across boundaries.

---

## 15. Explicit non-goals

- No tracing GC, ever.
- No exceptions / stack unwinding.
- No enum payloads or sum types — enums are named integer constants (C model).
- No class inheritance / implementation inheritance.
- No implicit aliasing or implicit heap allocation.
- No lifetime annotations or borrow operators in source syntax.
