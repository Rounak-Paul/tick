# Automatic Parallelization in TDL

How TDL automatically executes independent statements in parallel while maintaining determinism.

## The Problem with Traditional Threading

### Traditional Concurrent Programming

```cpp
// Java, C++, Go, Rust - all have this problem
int counter = 0;

thread_1: counter++;
thread_2: counter++;

// Result: Could be 1 or 2 (non-deterministic!)
```

Every run produces different results because threads interleave unpredictably:

```
Run 1: T1 reads 0, T2 reads 0, T1 writes 1, T2 writes 1 → Result: 1
Run 2: T1 reads 0, T2 reads 0, T2 writes 1, T1 writes 1 → Result: 1
Run 3: T1 reads 0, T1 writes 1, T2 reads 1, T2 writes 2 → Result: 2
```

**You never know what you'll get.**

## The TDL Solution: Automatic Safe Parallelization

### Deterministic Parallel Execution

```tdl
func main() {
  let a: int = 1 + 2;      // Runs in parallel
  let b: int = 3 + 4;      // Runs in parallel
  let c: int = a + b;      // Waits for a and b
  println(c);              // Output: 10 (always)
}
```

**Every execution produces identical results:**

```
Run 1: a and b compute in parallel → c = 10
Run 2: a and b compute in parallel → c = 10
Run 3: a and b compute in parallel → c = 10
```

**You always know exactly what you'll get.**

## How TDL Guarantees Determinism with Parallelization

### 1. Dependency Analysis

The compiler analyzes which variables each statement reads and writes:

```tdl
func compute() {
  let x: int = 10 + 20;    // Writes: {x}
  let y: int = 30 + 40;    // Writes: {y}
  let z: int = x + y;      // Reads: {x, y}, Writes: {z}
}
```

### 2. Execution Layers

Statements are grouped into "layers" where statements in the same layer don't depend on each other:

```
Layer 1: [x = 10 + 20] and [y = 30 + 40]  (independent, run in parallel)
         ↓
Layer 2: [z = x + y]  (depends on Layer 1, waits for it)
         ↓
Layer 3: [println(z)]
```

### 3. Deterministic Layer Ordering

Layers execute in a strict order:

```
Execution Timeline:
Time 1: Layer 1 starts
        ├─ Thread 1: x = 10 + 20
        └─ Thread 2: y = 30 + 40
        (layer completes when both threads finish)

Time 2: Layer 2 starts
        └─ z = x + y  (both x and y are ready)

Time 3: Layer 3 starts
        └─ println(z)
```

**The order is deterministic** - different runs produce the same layer execution order.

## Writing Parallelizable Code

### Independent Statements (Parallel)

Statements that don't depend on each other run in parallel:

```tdl
func parallel_computation() {
  let a: int = 100 + 200;     // Layer 1
  let b: int = 300 + 400;     // Layer 1 (independent of a)
  let c: int = 500 + 600;     // Layer 1 (independent of a and b)
  
  let sum: int = a + b + c;   // Layer 2 (depends on a, b, c)
  println(sum);               // Layer 3
}
```

**Execution:**
```
Layer 1: a, b, c compute on separate threads (parallel)
Layer 2: sum computes (waits for Layer 1)
Layer 3: println executes
```

### Dependent Statements (Sequential)

Statements that read variables written by other statements execute sequentially:

```tdl
func sequential_computation() {
  let total: int = 0;         // Layer 1
  total = total + 5;          // Layer 2 (reads total)
  total = total + 10;         // Layer 3 (reads total)
  println(total);             // Layer 4
}
```

**Execution:**
```
Layer 1: total = 0
Layer 2: total = total + 5  (waits for Layer 1)
Layer 3: total = total + 10 (waits for Layer 2)
Layer 4: println(total)     (waits for Layer 3)
```

### Mixed Parallelism

Combine parallel and sequential operations:

```tdl
func mixed_computation() {
  let a: int = 1 + 2;       // Layer 1
  let b: int = 3 + 4;       // Layer 1 (parallel with a)
  
  let c: int = a + 5;       // Layer 2 (depends on a)
  let d: int = b + 5;       // Layer 2 (depends on b, independent of c)
  
  let sum: int = c + d;     // Layer 3 (depends on c and d)
  println(sum);             // Layer 4
}
```

**Execution:**
```
Layer 1: a and b compute in parallel
Layer 2: c and d compute in parallel (both depend on Layer 1 but independent of each other)
Layer 3: sum computes (waits for Layer 2)
Layer 4: println executes
```

## Static Variables and Determinism

Static variables maintain state across function calls in a deterministic way:

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

**Deterministic guarantee:** Static variables always:
- Initialize to the same value on first call
- Increment/modify deterministically
- Produce identical sequences across runs

## Examples

### Example 1: Embarrassingly Parallel

```tdl
func square(int x) -> int {
  return x * x;
}

func main() {
  let a: int = square(2);    // Layer 1
  let b: int = square(3);    // Layer 1
  let c: int = square(4);    // Layer 1
  let d: int = square(5);    // Layer 1
  
  let sum: int = a + b + c + d;  // Layer 2
  println(sum);  // Output: 54
}
```

All four `square()` calls run in parallel in Layer 1.

### Example 2: Pipeline Pattern

```tdl
func stage1(int x) -> int {
  return x * 2;
}

func stage2(int x) -> int {
  return x + 10;
}

func stage3(int x) -> int {
  return x * 3;
}

func main() {
  let step1: int = stage1(5);      // Layer 1 (5*2 = 10)
  let step2: int = stage2(step1);  // Layer 2 (depends on step1)
  let result: int = stage3(step2); // Layer 3 (depends on step2)
  println(result);  // (10+10)*3 = 60
}
```

**Execution:**
```
Layer 1: stage1(5) → 10
Layer 2: stage2(10) → 20
Layer 3: stage3(20) → 60
```

### Example 3: Data Parallelism with Reduction

```tdl
func main() {
  // Independent computations (parallel)
  let sum1: int = 1 + 2;     // Layer 1
  let sum2: int = 3 + 4;     // Layer 1
  let sum3: int = 5 + 6;     // Layer 1
  let sum4: int = 7 + 8;     // Layer 1
  
  // Parallel reduction (parallel)
  let partial1: int = sum1 + sum2;  // Layer 2
  let partial2: int = sum3 + sum4;  // Layer 2
  
  // Final reduction (sequential)
  let total: int = partial1 + partial2;  // Layer 3
  
  println(total);  // Output: 36
}
```

**Performance:**
- Layer 1: 4 operations in parallel
- Layer 2: 2 operations in parallel
- Layer 3: 1 operation
- Total: Faster than sequential execution

## Performance Considerations

### Maximize Parallelization

**Good:** Many independent operations

```tdl
func good() {
  let a: int = compute1();
  let b: int = compute2();
  let c: int = compute3();
  let d: int = compute4();
  let result: int = a + b + c + d;  // All compute1-4 run in parallel
}
```

**Poor:** Long dependency chains

```tdl
func poor() {
  let a: int = compute();
  let b: int = a + compute();
  let c: int = b + compute();
  let d: int = c + compute();  // Sequential, can't parallelize
}
```

### Understand Layer Structure

Think about your data flow:

```
Good parallelization:       Poor parallelization:
┌─────┐                    ┌─────┐
│ Op1 │                    │ Op1 │
├─────┤                    ├─────┤
│ Op2 │ (parallel)         │ Op2 │ (sequential)
├─────┤                    ├─────┤
│ Op3 │                    │ Op3 │
├─────┤                    ├─────┤
│ Op4 │                    │ Op4 │
├─────────────────┐        ├─────┐
│   Reduce       │        │Result│
└─────────────────┘        └─────┘
```

## Determinism Guarantee

Every TDL program is deterministic because:

1. **No Race Conditions:** Parallelization is automatic and safe - only independent statements run in parallel
2. **Deterministic Ordering:** Layers execute in a strict, consistent order
3. **Deterministic Results:** Independent operations produce the same result every time
4. **No Randomness:** No random operations or non-deterministic sources

This makes TDL ideal for:
- **Scientific Computation:** Reproducible results
- **Financial Systems:** Auditable calculations
- **Simulation:** Repeatable experiments
- **Data Processing:** Consistent transformations
- **Debugging:** Every run looks the same

## Next Steps

- See [Language Reference](./language_reference.md) for syntax details
- Check [Getting Started](./getting_started.md) for practical examples
- Review [API Reference](./api_reference.md) for built-in functions
