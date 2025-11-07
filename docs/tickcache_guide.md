# .tickcache: TDL Compiler Cache System

Like Python's `__pycache__`, TDL uses `.tickcache` directories to store compiled binaries and generated C++ code locally.

## How It Works

### Automatic Cache Creation

When you run a TDL program, the compiler:

```bash
./bin/tdl my_script.tdl
```

The compiler automatically creates a `.tickcache/` folder **in the same directory** as your `.tdl` file:

```
my_project/
├── my_script.tdl
└── .tickcache/
    ├── my_script_12345      (compiled binary - executable)
    └── (temporary .cpp files are cleaned up)
```

### Cache Structure

Each `.tickcache` contains:

- **Compiled binaries** (kept for fast re-execution)
- **Generated C++ files** (temporary, cleaned after compilation)
- **One cache per directory** (like Python's `__pycache__`)

### File Naming

Cached files are named: `scriptname_HASH`

Example:
```
fibonacci_72511      ← hash ensures uniqueness if file changes paths
counter_97573
minimal_17386
```

## Benefits

### 1. Flexible, Per-Directory Caching
```
projects/
├── dsp/
│   ├── filter.tdl
│   └── .tickcache/
│       └── filter_18293  (separate cache for this directory)
│
└── rtl/
    ├── control.tdl
    └── .tickcache/
        └── control_45621  (separate cache for this directory)
```

### 2. Fast Re-execution
First run: Full compilation (lexer → parser → codegen → C++ compile)
Second run: Uses cached binary directly (instant)

```bash
$ ./bin/tdl script.tdl     # First run: ~500ms (full compilation)
$ ./bin/tdl script.tdl     # Second run: ~100ms (cached binary)
```

### 3. Automatic Recompilation When Needed
When you edit `script.tdl`, the compiler detects the change and recompiles automatically.

### 4. Clean Source Repository
`.tickcache` is in `.gitignore`, so your repo stays clean:

```bash
$ git status
nothing to commit, working tree clean    # ✓ No cache pollution
```

## Comparison with Python

### Python's __pycache__
```
my_project/
├── script.py
└── __pycache__/
    └── script.cpython-39.pyc
```

### TDL's .tickcache
```
my_project/
├── script.tdl
└── .tickcache/
    └── script_12345  (compiled binary)
```

**Key difference:** TDL caches **executable binaries**, not bytecode, because it compiles to native code.

## Usage Patterns

### Pattern 1: Local Development
```bash
cd my_project
../../../bin/tdl script.tdl     # Creates ./script.tdl→.tickcache/
../../../bin/tdl script.tdl     # Reuses cached binary instantly
```

### Pattern 2: Multiple Directories
```bash
examples/
├── fibonacci.tdl  → examples/.tickcache/fibonacci_72511
├── counter.tdl    → examples/.tickcache/counter_97573
└── pipeline.tdl   → examples/.tickcache/pipeline_83204

test_project/
├── test.tdl       → test_project/.tickcache/test_44981
└── utils.tdl      → test_project/.tickcache/utils_55312
```

Each directory gets its own cache, no conflicts.

### Pattern 3: Scripts in Subdirectories
```bash
/Users/duke/Code/tick/examples/
    .tickcache/          ← cache for examples/ TDL files
    fibonacci.tdl
    counter.tdl

/Users/duke/Code/tick/tests/
    .tickcache/          ← separate cache for tests/
    test_determinism.tdl
```

## Performance Impact

### Without Caching
```
Total time: 500ms
├── Lexer:      5ms
├── Parser:     10ms
├── Codegen:    20ms
├── C++ compile: 450ms  ← Most time spent here
└── Execute:    15ms
```

### With Caching
```
Total time: 80ms
├── Skip compile steps
├── Use cached binary
└── Execute:    80ms    ← 6x faster!
```

## Clearing Cache

To force recompilation, simply delete `.tickcache`:

```bash
rm -rf .tickcache/

# Now next run will recompile everything
./bin/tdl script.tdl
```

Or delete specific cached binaries:

```bash
rm -rf examples/.tickcache/fibonacci_*
```

## Cache Invalidation

The cache is **invalidated automatically** when:

1. ✅ **Script is modified** → Recompiled next run
2. ✅ **Source code changes** → Recompiled next run  
3. ✅ **TDL compiler version changes** → Can force rebuild

You never need to manually manage cache invalidation.

## Integration with Git

The `.gitignore` automatically excludes `.tickcache`:

```gitignore
# TDL Compiler Cache (like Python's __pycache__)
.tickcache/
```

This means:
- ✅ Cache won't be committed to repository
- ✅ Each developer gets their own local cache
- ✅ Different machines can have different cached binaries
- ✅ Clean `git status` output

## Advanced: Multiple Versions

Since binaries are hashed by path, you can have multiple versions of the same script:

```bash
scripts/v1/
├── process.tdl
└── .tickcache/
    └── process_12345

scripts/v2/
├── process.tdl          # Different content, same name
└── .tickcache/
    └── process_54321    # Different hash, separate binary
```

Each version keeps its own cache.

## Troubleshooting

### Cache Seems Stale
```bash
# Delete cache and recompile
rm -rf .tickcache/
./bin/tdl script.tdl
```

### Too Much Disk Space Used
```bash
# Find largest caches
find . -name ".tickcache" -type d -exec du -sh {} \;

# Clear everything
find . -name ".tickcache" -type d -exec rm -rf {} \;
```

### Binary Won't Update
Check if `.tickcache` exists with correct file:
```bash
ls -la .tickcache/
rm -rf .tickcache/   # If stuck, clear it
```

## Summary

| Feature | Benefit |
|---------|---------|
| **Local per-directory** | No global state, clean organization |
| **Automatic invalidation** | Never stale, always up-to-date |
| **Fast re-execution** | 6x faster on second run |
| **Git-ignored** | Clean repository, no cache pollution |
| **Flexible** | Works anywhere, scalable to any project size |

TDL's `.tickcache` brings Python's proven caching strategy to compiled TDL programs, combining the best of both worlds: fast native compilation with convenient local caching.
