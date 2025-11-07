# Understanding Deterministic Parallelism

Core concepts behind TDL's deterministic concurrency model.

## The Problem with Traditional Threading

### Traditional Approach (Race Conditions)

```cpp
// Java, C++, Go, Rust all have this problem
int counter = 0;

// Thread 1: counter++
// Thread 2: counter++

// Result: Could be 1 or 2 (non-deterministic!)
```

Every run produces different results because threads interleave unpredictably:

```
Run 1: T1 reads 0, T2 reads 0, T1 writes 1, T2 writes 1 → Result: 1
Run 2: T1 reads 0, T2 reads 0, T2 writes 1, T1 writes 1 → Result: 1  (same by luck)
Run 3: T1 reads 0, T1 increments, writes 1, T2 reads 1, increments, writes 2 → Result: 2
```

**You never know what you'll get.**

## The TDL Solution

### Deterministic Model

```tdl
clock sys = 100hz;

proc p1(chan<int> out) {
  on sys.tick {
    static x: int = 0;
    x = x + 1;
    out.send(x);
  }
}

proc p2(chan<int> out) {
  on sys.tick {
    static x: int = 0;
    x = x + 1;
    out.send(x);
  }
}
```

**Every execution produces identical results:**

```
Run 1: p1 outputs 1,2,3...  p2 outputs 1,2,3...
Run 2: p1 outputs 1,2,3...  p2 outputs 1,2,3...
Run 3: p1 outputs 1,2,3...  p2 outputs 1,2,3...
```

**You always know exactly what you'll get.**

## How TDL Guarantees Determinism

### 1. Clock-Synchronized Execution

All processes synchronize to clock ticks:

```
Clock Tick 0:
  ├─ Process A executes (deterministic)
  ├─ Process B executes (deterministic)
  └─ Process C executes (deterministic)

Clock Tick 1:
  ├─ Process A executes (deterministic)
  ├─ Process B executes (deterministic)
  └─ Process C executes (deterministic)
```

**Key:** At each tick, processes execute in the same order, always.

### 2. Process Isolation (No Shared Memory)

Processes cannot share memory:

```tdl
// ❌ NOT ALLOWED - processes are isolated
static shared_counter: int = 0;  // Would be process-local

proc p1() {
  on sys.tick {
    static my_counter: int = 0;  // ✓ Only this process sees this
    my_counter = my_counter + 1;
  }
}

proc p2() {
  on sys.tick {
    static my_counter: int = 0;  // ✓ Only this process sees this
    my_counter = my_counter + 1;
  }
}
```

**Each process has isolated state.** No races possible.

### 3. Channel-Based Communication

Only way to communicate: channels

```tdl
proc producer(chan<int> out) {
  on sys.tick {
    out.send(data);  // ✓ Deterministic send
  }
}

proc consumer(chan<int> in) {
  on sys.tick {
    // ✓ Deterministic receive (FIFO)
  }
}
```

**Channels guarantee:**
- FIFO ordering (first sent, first received)
- No interleaving of messages
- Deterministic delivery

### 4. Static Variables Persist

State survives ticks:

```tdl
proc accumulator(chan<int> out) {
  on sys.tick {
    static sum: int = 0;    // ✓ Persists across ticks
    sum = sum + 10;
    out.send(sum);
  }
}
```

**Output:** 10, 20, 30, 40, 50...

**Always the same sequence, every run.**

## Execution Model

### One Tick

```
on sys.tick {
  println("A");  // 1
  println("B");  // 2
}
```

Executes in order: 1, then 2. Always.

### Multiple Processes, One Tick

```tdl
proc p1() { on sys.tick { println("P1"); } }
proc p2() { on sys.tick { println("P2"); } }
```

Both run every tick. Order is deterministic (alphabetical by default):
```
P1
P2
P1
P2
...
```

### Multiple Ticks

```
Tick 0: [All processes execute]
Tick 1: [All processes execute]
Tick 2: [All processes execute]
```

Each tick is independent but deterministic.

## Race-Free Guarantees

### No Race Conditions

```tdl
// ❌ Traditional threading - RACE CONDITION
x++  // Thread 1
x++  // Thread 2
// Result: could be 1 or 2

// ✓ TDL - DETERMINISTIC
static x: int = 0;
proc p1() { on sys.tick { x = x + 1; } }
proc p2() { on sys.tick { x = x + 1; } }
// Result: p1 always outputs 1,2,3... and p2 always outputs 1,2,3...
```

### No Deadlocks

```tdl
// ✓ No locks, so no deadlocks
proc p1(chan<int> to_p2, chan<int> from_p2) {
  on sys.tick {
    to_p2.send(value);
    // Other process receives deterministically
  }
}

proc p2(chan<int> to_p1, chan<int> from_p1) {
  on sys.tick {
    to_p1.send(value);
    // Other process receives deterministically
  }
}
```

### No Lost Updates

```tdl
// ✓ Static variables are atomic (per tick)
static counter: int = 0;

proc increment() {
  on sys.tick {
    counter = counter + 1;  // Always executes completely
  }
}
```

## Parallelism Without Synchronization

### True Parallelism

```tdl
clock fast;

proc heavy_compute(chan<int> out) {
  on fast.tick {
    static i: int = 0;
    println(i);
    i = i + 1;
  }
}

proc io_operation(chan<int> out) {
  on fast.tick {
    static j: int = 100;
    println(j);
    j = j + 1;
  }
}

proc data_logging(chan<int> out) {
  on fast.tick {
    static k: int = 200;
    println(k);
    k = k + 1;
  }
}
```

**All three run in parallel every tick.**

```
Tick 0: [A runs] [B runs] [C runs] (parallel)
Tick 1: [A runs] [B runs] [C runs] (parallel)
Tick 2: [A runs] [B runs] [C runs] (parallel)
```

**Zero overhead:**
- No mutex locks
- No condition variables
- No atomic operations
- No synchronization primitives

### Automatic Load Balancing

Processes complete whenever done, then move to next tick:

```tdl
proc fast_process(chan<int> out) {
  on sys.tick {
    println("fast");
  }
}

proc slow_process(chan<int> out) {
  on sys.tick {
    static i: int = 0;
    while (i < 1000000) { i = i + 1; }
    println("slow");
  }
}
```

**Tick waits for slowest process** (deterministic).

## Provable Correctness

### Identical Output Every Run

```bash
$ ./build/bin/tdl program.tdl
Output: 1, 10, 1, 2, 20, 2, 3, 30, 3...

$ ./build/bin/tdl program.tdl
Output: 1, 10, 1, 2, 20, 2, 3, 30, 3...

$ ./build/bin/tdl program.tdl
Output: 1, 10, 1, 2, 20, 2, 3, 30, 3...
```

**This proves correctness.**

### Verifiable Execution

Check statistics to verify determinism:

```
=== Statistics ===
Ticks executed: 10
Average slack: 19.95 ms
Min slack: 19.91 ms
Max slack: 20.01 ms
```

**Consistent slack means deterministic execution.**

## Comparison: TDL vs Traditional Concurrency

| Aspect | Traditional | TDL |
|--------|-------------|-----|
| Race conditions | ✗ Unavoidable | ✓ Impossible |
| Determinism | ✗ Non-deterministic | ✓ Guaranteed |
| Synchronization | ✓ Locks, mutexes | ✓ Built-in |
| Performance | ✗ Lock contention | ✓ Zero overhead |
| Deadlocks | ✗ Possible | ✓ Impossible |
| Correctness | ✗ Hard to prove | ✓ Automatic |
| Debugging | ✗ Heisenbugs | ✓ Reproducible |

## Real-World Impact

### Traditional Concurrency Problem

```
Development:  "Works fine on my machine!"
Testing:      "Works fine here too"
Production:   "CRASHES RANDOMLY"
Investigation: "Only happened once at 3am"
```

### TDL Solution

```
Development:  "Works fine on my machine!"
Testing:      "Works fine here too"
Production:   "Works exactly the same way"
Investigation: "Output is reproducible, always"
```

## Key Takeaway

**TDL proves that deterministic parallelism is not only possible but practical.**

Unlike traditional threading:
- ✓ Every execution identical
- ✓ No race conditions
- ✓ No manual synchronization
- ✓ No deadlocks
- ✓ Provably correct

This is the future of concurrent programming.

## Next: See [Clock Modes](./clock_modes.md)
