# Getting Started with TDL

Step-by-step guide to write and run your first TDL program.

## Installation & Setup

### 1. Build the Compiler

```bash
cd /Users/duke/Code/tick
cmake . && make
```

The compiler `tdl` will be at `build/bin/tdl`.

### 2. Verify Installation

```bash
./build/bin/tdl --version  # May not show version, but confirms executable exists
ls -l ./build/bin/tdl      # Should show executable
```

## Your First Program

### Simple Clock Example

Create a file `hello.tdl`:

```tdl
clock sys = 10hz;

proc greet() {
  on sys.tick {
    println("Hello from TDL!");
  }
}
```

### Run It

```bash
./build/bin/tdl hello.tdl
```

**Output:**
```
Hello from TDL!
Hello from TDL!
Hello from TDL!
Hello from TDL!
Hello from TDL!
Hello from TDL!
Hello from TDL!
Hello from TDL!
Hello from TDL!
Hello from TDL!

=== Statistics ===
Clock: sys
Frequency: 10 Hz
Period: 100 ms
Ticks executed: 10
Average slack: 99.99 ms
Min slack: 99.98 ms
Max slack: 100.01 ms
```

Notice: 10 outputs (10 ticks), each tick waits ~100ms (1000ms / 10Hz).

## Step 1: Using a Clock

```tdl
clock tick = 5hz;  // 5 ticks per second = 200ms per tick

proc counter() {
  on tick.tick {
    static i: int = 0;
    println(i);
    i = i + 1;
  }
}
```

**Output:** 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 (one per 200ms)

**What happened:**
- Defined a 5 Hz clock (200ms per tick)
- Created a process with static variable
- Every tick, increment and print
- Runs 10 ticks (default)

## Step 2: Using Channels

```tdl
clock sys = 10hz;

proc producer(chan<int> out) {
  on sys.tick {
    static i: int = 0;
    println(i);
    out.send(i);
    i = i + 1;
  }
}

proc consumer(chan<int> in) {
  on sys.tick {
    println(100);
  }
}
```

**Output:**
```
0
100
1
100
2
100
...
```

**What happened:**
- Producer sends integers to channel
- Consumer receives (implicitly)
- Both run every tick, deterministically, in parallel

## Step 3: Using Functions

```tdl
func fibonacci(int n) -> int {
  if (n <= 1) {
    return n;
  }
  return fibonacci(n - 1) + fibonacci(n - 2);
}

func main() {
  let result: int = fibonacci(10);
  println(result);
}
```

**Output:** `55` (instantly, no clock needed)

**What happened:**
- Defined recursive fibonacci function
- Called it with argument 10
- Got 55 back (10th fibonacci number)

## Step 4: Deterministic Parallelism

This is TDL's superpower. Multiple processes run in perfect parallel:

```tdl
clock fast;  // Max speed, no delays

proc counter1(chan<int> out) {
  on fast.tick {
    static i: int = 0;
    println(i);
    i = i + 1;
  }
}

proc counter2(chan<int> out) {
  on fast.tick {
    static j: int = 100;
    println(j);
    j = j + 1;
  }
}

proc counter3(chan<int> out) {
  on fast.tick {
    static k: int = 200;
    println(k);
    k = k + 1;
  }
}
```

**Output:**
```
0
100
200
1
101
201
2
102
202
...
```

**Key point:**
- Three processes run in **perfect parallel**
- Every tick: all three execute deterministically
- Run this 100 times: **identical output all 100 times**
- **No race conditions, no synchronization needed**

This is what makes TDL revolutionary.

## Step 5: Static State Management

Static variables persist across ticks:

```tdl
clock sys = 5hz;

proc accumulator(chan<int> out) {
  on sys.tick {
    static sum: int = 0;
    sum = sum + 10;
    println(sum);
    out.send(sum);
  }
}
```

**Output:** 10, 20, 30, 40, 50, 60, 70, 80, 90, 100

**What happened:**
- Static `sum` starts at 0
- Each tick: add 10
- Each tick's value persists to next tick

## Step 6: Control Flow

```tdl
clock fast;

proc loop_demo(chan<int> out) {
  on fast.tick {
    static counter: int = 0;
    
    if (counter < 3) {
      println(counter);
    }
    
    counter = counter + 1;
  }
}
```

**Output:** 0, 1, 2 (then stops printing)

## Step 7: Reading Statistics

Every run shows timing statistics:

**Frequency-based:**
```
=== Statistics ===
Clock: sys
Frequency: 10 Hz
Period: 100 ms
Ticks executed: 10
Average slack: 99.95 ms
Min slack: 99.91 ms
Max slack: 100.02 ms
```

**Max-speed:**
```
=== Statistics ===
Clock: fast
Mode: MAX SPEED (no sleep delays)
Ticks executed: 10
Average slack: -0.005 ms
Min slack: -0.015 ms
Max slack: -0.001 ms
```

**What it means:**
- **Positive slack** = time we slept
- **Negative slack** = execution was faster (max-speed)
- Consistent slack = deterministic execution

## Common Tasks

### Print a Value

```tdl
println(42);
println("hello");
println(3.14);
```

### Count from 0 to 9

```tdl
clock sys = 100hz;

proc counter(chan<int> out) {
  on sys.tick {
    static i: int = 0;
    println(i);
    i = i + 1;
  }
}
```

### Send Data Through Channels

```tdl
proc source(chan<int> out) {
  on sys.tick {
    out.send(42);
  }
}

proc sink(chan<int> in) {
  on sys.tick {
    println(100);
  }
}
```

### Run Multiple Processes in Parallel

```tdl
proc p1() { on sys.tick { println("A"); } }
proc p2() { on sys.tick { println("B"); } }
proc p3() { on sys.tick { println("C"); } }
```

All three run in deterministic parallel every tick.

### Call a Function

```tdl
func add(int a, int b) -> int {
  return a + b;
}

proc worker(chan<int> out) {
  on sys.tick {
    let result: int = add(10, 20);
    out.send(result);
  }
}
```

### Conditional Logic

```tdl
proc conditional(chan<int> out) {
  on sys.tick {
    static counter: int = 0;
    
    if (counter % 2 == 0) {
      println("even");
    }
    
    counter = counter + 1;
  }
}
```

## Running Examples

Try the included examples:

```bash
./build/bin/tdl examples/fibonacci.tdl
./build/bin/tdl examples/deterministic_accumulator.tdl
./build/bin/tdl examples/max_speed.tdl
./build/bin/tdl examples/counter.tdl
```

## Next Steps

1. **Read** [Language Reference](./language_reference.md) for complete syntax
2. **Learn** [Deterministic Parallelism](./parallelism_guide.md) concepts
3. **Understand** [Clock Modes](./clock_modes.md)
4. **Explore** examples in `examples/` directory

## Troubleshooting

### Parse Error

**Problem:** `Parse error at line X, column Y`

**Solution:** Check syntax matches language reference. Common issues:
- Missing semicolons
- Unmatched braces
- Wrong clock name

### Compile Error (in generated C++)

**Problem:** `C++ compilation failed`

**Solution:** Usually means invalid operation. Check:
- Type mismatches
- Undefined variables
- Wrong function signatures

### No Output

**Problem:** Program runs but produces no output

**Solution:**
- Add `println()` statements to processes
- Check if processes are on correct clock
- Verify clock is named correctly

### Unexpected Results

**Problem:** Output doesn't match expectations

**Solution:**
- Check if using static variables (to persist state)
- Verify clock frequency
- Test with simpler examples first
