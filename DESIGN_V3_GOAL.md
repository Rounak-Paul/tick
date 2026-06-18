# Tick V3 — Feature Checklist

This document is the implementation checklist. Each item is a discrete unit of work.
Check it off when fully implemented, tested, and reflected in the compiler + test suite.
No item is added without a concrete reason. No feature is added just because other
languages have it.

---

## Core language (already implemented)

- [x] Primitives: `i8 i16 i32 i64 u8 u16 u32 u64 f32 f64 bool str void`
- [x] `var` — single binding keyword, mutable by default
- [x] `const` — type qualifier for immutable bindings (`var x : const i32 = 10`)
- [x] `struct` — value type, C struct layout, no inheritance
- [x] `impl T` — methods on a type
- [x] `self` — value receiver (copy)
- [x] `ref self` — borrow receiver (may read or mutate; function's own concern)
- [x] `enum` — named integer constants, C model, optional explicit values
- [x] `match` — exhaustive pattern matching on enum variants
- [x] `interface` — contract (set of method signatures), static dispatch
- [x] `impl I for T` — declare type satisfies interface (compiler verifies)
- [x] `ref T` — borrow parameter (pass by pointer, no copy, no call-site annotation)
- [x] `shared T` — reference-counted heap ownership
- [x] `weak T` — non-owning pointer to a shared object (cycle breaker)
- [x] `T[]` — dynamic array (heap buffer, stack handle)
- [x] `T[N]` — fixed-size inline array (stack)
- [x] `str` — owned UTF-8 string (heap buffer, stack handle)
- [x] Ownership inference — compiler inserts frees at last-use / scope exit
- [x] Move elision — last-use value pass lowers to move, not copy
- [x] `for i in 0..n` — half-open range loop
- [x] `for x in items` — iterate array by value (copy)
- [x] `for ref x in items` — iterate array by borrow
- [x] `while`, `if/else if/else`, `break`, `continue`, `return`
- [x] `defer` — runs at scope exit, LIFO, for effectful teardown only
- [x] `unsafe { }` — raw pointer operations, FFI
- [x] `extern func` / `link` — C FFI declarations
- [x] `cast(expr, T)` — explicit type cast
- [x] `sizeof(T)` — size of type in bytes
- [x] `signal` / `event` / `process` — deterministic concurrency primitives
- [x] `pub` — visibility modifier (parsed; enforcement deferred to modules)
- [x] Three build modes: default (bounds), `--validate` (all checks), `--release` (none)
- [x] Codegen to C, compiled via gcc/clang

---

## Must-have to not be a toy

- [ ] **Modules** — `import mod`, `from mod import name`, `pub` enforcement
  - Each `.tick` file is a module
  - `pub` items are importable; non-`pub` items are private
  - Compiler resolves imports before type-checking
  - No circular imports (error at compile time)

- [ ] **Minimal I/O stdlib** — without this nothing useful can be written
  - `print(str)` / `println(str)` — already exists as builtin, make it proper
  - `read_line() : str` — read a line from stdin
  - File I/O: `open(path, mode) : File`, `read(f) : str`, `write(f, s)`, `close(f)`
  - `File` is a struct with a `drop` method (called at scope exit automatically)

- [ ] **`drop` method** — custom teardown called by compiler at reclaim point
  - `impl T { func drop(ref self) { ... } }`
  - Called automatically when an owned value goes out of scope
  - Required for File, Socket, and any resource-owning struct

- [ ] **Multiple return via struct** — already possible, but needs convention + docs
  - No language change needed; document the pattern clearly

---

## Quality-of-life (do after must-haves)

- [ ] **`dyn I` dispatch** — dynamic dispatch via fat pointer (data + vtable)
  - Already parsed and type-checked; codegen not implemented
  - Required for heterogeneous collections and plugin-style APIs

- [ ] **Global `const`** — compile-time constants at file scope
  - `var MAX : const i32 = 1024` at top level already works; verify and document

- [ ] **Named struct constructors** — `T { field = value }` already works; document

- [ ] **`cast` documentation** — already implemented, not documented in design

- [ ] **`sizeof` documentation** — already implemented, not documented in design

- [ ] **Fixed array iteration** — `for x in arr` where arr is `T[N]`
  - Currently only dynamic arrays are iterable

- [ ] **Fixed array length** — `arr.len()` for `T[N]` (compile-time constant)

---

## Explicitly not in scope (will not be added)

- Garbage collector
- Exceptions / stack unwinding
- Enum payloads / sum types / `Result<T>` / `T?`
- Class inheritance
- Implicit aliasing or implicit heap allocation
- Lifetime annotations in source syntax
- Generics / monomorphization (use concrete types; generics add massive complexity)
- Operator overloading
- Closures / lambdas
- Variadic functions (beyond builtin `println`)
- Reflection / runtime type info
- Async / await (signals/processes cover the concurrency use case)
