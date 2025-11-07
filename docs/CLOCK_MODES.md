# Clock Modes

Understanding TDL's two clock modes for different execution needs.

## Two Clock Modes

TDL provides two ways to run your programs:
1. **Frequency-based** - Enforces timing constraints
2. **Max-speed** - Runs as fast as possible

## Frequency-Based Clock

### Definition

```tdl
clock sys = 100hz;      // 100 ticks per second
clock tick = 50hz;      // 50 ticks per second
clock heartbeat = 1hz;  // 1 tick per second
```

**Behavior:**
- Each tick takes exactly 20ms (1000ms / 50Hz)
- If processing finishes early, clock sleeps to maintain timing
- If processing takes longer, negative slack is recorded
- **Use case:** Real-time systems, hardware interfacing, deterministic timing requirements

**Example output:**
```
=== Statistics ===
Clock: sys
Frequency: 50 Hz
Period: 20 ms
Ticks executed: 10
Average slack: 19.98 ms
Min slack: 19.97 ms
Max slack: 19.99 ms
```

(Note: Positive slack means we finished early and slept)

## Max-Speed Clock (no timing constraints)

When you don't specify a frequency, the clock runs at **maximum speed**:

```tdl
clock fast;  // No frequency - runs as fast as possible!
```

**Behavior:**
- No artificial delays or sleep
- Each tick completes immediately after all processes finish
- Maximizes throughput
- Negative slack indicates pure execution time (no sleep)
- **Use case:** Batch processing, simulation, algorithm verification, maximum throughput

**Example output:**
```
=== Statistics ===
Clock: fast
Mode: MAX SPEED (no sleep delays)
Ticks executed: 10
Average slack: -0.006 ms
Min slack: -0.024 ms
Max slack: -0.002 ms
```

(Note: Negative slack shows we ran faster than sleeping would allow)

## Comparison

| Aspect | Frequency Clock | Max-Speed Clock |
|--------|-----------------|-----------------|
| Syntax | `clock sys = 100hz;` | `clock fast;` |
| Timing | Enforced | No delays |
| Sleep | Yes (maintains period) | No (immediate) |
| Use Case | Real-time systems | Batch/simulation |
| Throughput | Limited by frequency | Maximum |
| Slack | Positive (early finish time) | Negative (pure execution) |
| Determinism | ✅ Clock-enforced | ✅ Order-enforced |

## Example: Same Code, Two Modes

**With Frequency (10 Hz):**
```tdl
clock sys = 10hz;

proc counter(chan<int> out) {
  on sys.tick {
    static i: int = 0;
    println(i);
    i = i + 1;
  }
}
```

Execution takes ~1 second for 10 ticks (100ms per tick)

**Max Speed (No Frequency):**
```tdl
clock fast;

proc counter(chan<int> out) {
  on fast.tick {
    static i: int = 0;
    println(i);
    i = i + 1;
  }
}
```

Execution takes ~milliseconds for 10 ticks

**Both produce identical output but at different speeds.**

## Key Insight

The **deterministic parallelism guarantee is independent of clock speed**:
- Frequency clock: Deterministic timing + deterministic ordering
- Max-speed clock: Deterministic ordering (no timing constraint)

Both ensure **identical execution every run with zero race conditions**.

This is what makes TDL revolutionary: you get parallelism correctness regardless of timing, and you can choose your clock speed independently.
