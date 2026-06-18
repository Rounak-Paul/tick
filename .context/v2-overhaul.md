# Tick 2.0 Overhaul

Complete rewrite to a value-semantics, ownership-inferred systems language.
Vision + full spec: `DESIGN_V2.md`. User-facing docs: `README.md`.

## Core model
- Value-by-default. Aliasing only via explicit `ref` (borrow) or `shared` (refcount).
- `var` is the single binding keyword. `const` is a type qualifier: `var x : const i32 = 10` makes the binding immutable (zero cost, statically enforced).
- `ref T` = single borrow form (pass by pointer, no copy). Whether the function mutates is its own concern — C model. No call-site annotation needed.
- Owned heap data (`str`, `T[]`, `shared`) reclaimed at compiler-chosen points; no GC.
- Validation layer = build mode: default (bounds), `--validate` (all), `--release` (none).
- `enum` = named integer constants only (C model). No payloads, no sum types, no `Result<T>`, no `T?`.
- Error handling: return a struct or use an out param via `ref var`. No exceptions.

## Pipeline & files
`lexer -> parser -> checker -> codegen -> cc` (driver orchestrates).

| File | Role |
|------|------|
| `src/compiler/token.h` | token kinds |
| `src/compiler/lexer.{h,cpp}` | source -> tokens |
| `src/compiler/type.h` | `TypeRef` (kind + `Ownership`), the cost/aliasing source of truth |
| `src/compiler/ast.h` | AST nodes |
| `src/compiler/parser.{h,cpp}` | tokens -> AST. `_no_struct_lit` flag disambiguates `Name {` literal vs block in if/while/for conditions |
| `src/compiler/checker.{h,cpp}` | types, interface satisfaction, match exhaustiveness, mutability, ownership decisions (move/reclaim side-tables keyed by node ptr) |
| `src/runtime/build_mode.h` | `BuildMode` enum |
| `src/runtime/codegen.{h,cpp}` | AST -> C. Per-scope reclaim + defer (LIFO) lists; value-copy on bind (array deep copy, str dup, shared retain) |
| `src/runtime/driver.{h,cpp}` | pipeline + invokes `cc`, locates runtime dir |
| `src/runtime/tick_runtime.{c,h}` | TickArray, shared/weak refcount, signals/events, str ops, validation traps |
| `src/core/*` | String, DynamicArray, HashMap (reused from v1) |

## Memory soundness (the key invariants)
- Struct assignment / `var b = a` = value copy (no aliasing).
- Array bind from lvalue -> `tick_array_copy` (deep). String bind from lvalue -> `tick_str_dup`.
- `shared` bind from existing handle -> `tick_shared_retain`; freshly boxed `shared Expr{}` starts at rc 1.
- String binding reclaimed only when init is heap-producing (NOT a string literal -> static).
- `ref`/`weak` never reclaimed.
- Verified: every test passes under ASan+UBSan in the suite runner.

## Implemented features
primitives, structs+impl+methods (`self`/`ref self`/`ref var self`), interfaces
(static), plain enums (int constants) + exhaustive `match`, dynamic arrays (`push`/
`pop`/`len`/index/`for-in`/`for ref`), fixed arrays, strings (`+`/`==`/`len`/
`str_order`/`to_str`), `ref`/`shared`/`weak`, `var` + `const` type qualifier, control flow, `defer`,
`unsafe` + `extern`/`link`, signals/events/processes, globals, `cast`/`sizeof`,
three build modes.

## Deliberately deferred (NOT implemented; remove from DESIGN scope if not pursued)
- Modules / `import` / `pub` enforcement (parser has no import; `pub` parsed, not enforced).
- `dyn Interface` runtime dispatch (type exists, no vtable codegen yet).
- Auto-promote-to-shared inference + perf notes (move-elision is the simple last-use form).

## Explicit non-goals (will not be added)
- Sum-type enums with payloads — enums are C-style named integers only.
- `Result<T>`, `T?` optionals, `?`/`!` operators — no special error type machinery.
- Exceptions or stack unwinding.

## Build & test
`cmake -B build && cmake --build build` (warning-free under -Wall).
`bash tests/run_test_suite.sh` — 9 tests, each run in default + release + ASan/UBSan.
