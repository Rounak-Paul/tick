# TDL Examples

This directory contains organized examples demonstrating TDL language features and capabilities.

## Directory Structure

### `basics/` - Fundamental Language Features

Start here if you're new to TDL.

- **01_hello_world.tdl** - Simple output with println
- **02_variables.tdl** - Variable declarations and arithmetic
- **03_if_else.tdl** - Conditional control flow
- **04_while_loop.tdl** - Loop control flow
- **05_recursion.tdl** - Function recursion and factorial
- **06_static_variables.tdl** - State maintenance across calls

### `intermediate/` - Practical Patterns

Demonstrate common programming patterns in TDL.

- **01_fibonacci.tdl** - Classic recursive Fibonacci
- **02_parallel_basic.tdl** - Basic automatic parallelization
- **03_parallel_map_reduce.tdl** - Parallel map-reduce pattern
- **04_deterministic_accumulation.tdl** - State accumulation
- **05_pipeline.tdl** - Multi-stage sequential pipeline

### `advanced/` - Complex Scenarios

Advanced patterns and optimizations.

- **01_large_parallel_sum.tdl** - Large-scale parallel computation
- **02_mixed_parallelism.tdl** - Mixed parallel and sequential code
- **03_recursive_computation.tdl** - Recursion with parallelization
- **04_deterministic_state.tdl** - Complex state management

## Running Examples

To run any example:

```bash
./build/bin/tdl examples/basics/01_hello_world.tdl
```

## Learning Path

1. **Start with basics/** - Learn the syntax and core features
2. **Progress to intermediate/** - Understand parallelization patterns
3. **Explore advanced/** - See complex real-world patterns

## Key Concepts

### Automatic Parallelization

Independent statements run in parallel automatically:

```tdl
let a: int = 10 + 20;  // Parallel
let b: int = 30 + 40;  // Parallel
let c: int = a + b;    // Waits for a and b
```

### Deterministic Execution

Every execution produces identical results:

```tdl
func counter() -> int {
  static count: int = 0;
  count = count + 1;
  return count;
}

// Always outputs: 1, 2, 3, ...
```

### No Race Conditions

Parallelization is automatic and safe - you can never create race conditions.

## Tips

- Start simple and build complexity gradually
- Use static variables for state that needs to persist
- Think about data dependencies when designing algorithms
- Leverage parallelization for compute-intensive operations

## Next Steps

- Read the [Language Reference](../docs/language_reference.md)
- Explore the [Parallelism Guide](../docs/parallelism_guide.md)
- Check the [API Reference](../docs/api_reference.md)
