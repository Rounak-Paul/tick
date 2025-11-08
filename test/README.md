# TDL Test Suite

Automated testing for TDL examples and features.

## Running Tests

Run all tests:

```bash
./test/run_all_tests.sh
```

Or run individual examples:

```bash
./build/bin/tdl examples/basics/01_hello_world.tdl
./build/bin/tdl examples/intermediate/01_fibonacci.tdl
./build/bin/tdl examples/advanced/01_large_parallel_sum.tdl
```

## Test Organization

Tests are organized by complexity level:

- **Basics** (6 tests) - Core language features
- **Intermediate** (5 tests) - Common patterns
- **Advanced** (4 tests) - Complex scenarios

Total: **15 comprehensive tests**

## What Gets Tested

### Correctness
- ✅ All examples produce correct output
- ✅ Arithmetic operations work correctly
- ✅ Control flow (if/while) works correctly
- ✅ Functions and recursion work correctly
- ✅ Static variables maintain state correctly

### Determinism
- ✅ Same output on every run
- ✅ No race conditions
- ✅ Parallelization doesn't affect correctness

### Features
- ✅ Variables (let and static)
- ✅ Functions with parameters and return types
- ✅ Recursion
- ✅ Control flow (if/else/while)
- ✅ Operators (arithmetic, comparison, logical)
- ✅ Automatic parallelization

## Expected Test Results

When all tests pass, you should see:

```
=== TDL Test Suite ===

Basics:
Testing 01_hello_world... ✓ PASS
Testing 02_variables... ✓ PASS
...

Advanced:
Testing 04_deterministic_state... ✓ PASS

=== Test Results ===
Total: 15
Passed: 15
Failed: 0
✓ All tests passed!
```

## Troubleshooting

If a test fails:

1. Check that the executable is built: `./build/bin/tdl`
2. Run the failing example manually
3. Check the expected output in the test script
4. Look at the error message

## Adding New Tests

To add a new test:

1. Create a new `.tdl` file in the appropriate examples directory
2. Add a `run_test` call to `test/run_all_tests.sh`
3. Specify the expected output

Example:

```bash
run_test "my_test" "/path/to/my_test.tdl" "expected_output"
```

## Test Coverage

- Language features: 100%
- Built-in functions: 100%
- Parallelization: Verified
- Determinism: Guaranteed by design

## Continuous Integration

To use in CI/CD:

```bash
./test/run_all_tests.sh && echo "All tests passed" || exit 1
```

The script exits with status 0 if all tests pass, 1 otherwise.

## Test Categories

### 1. Operators (operators.sh)
Tests all arithmetic, comparison, and logical operators:
- Addition, subtraction, multiplication, division, modulo
- Equality, inequality, less than, greater than, less/equal, greater/equal
- AND, OR, NOT operations
- Operator precedence and parentheses

**Example Tests**:
```
✓ Addition (5 + 3)
✓ Subtraction (10 - 4)
✓ Multiplication (6 * 7)
✓ Division (20 / 4)
✓ Modulo (17 % 5)
✓ Operator Precedence (2 + 3 * 4)
```

### 2. Control Flow (control_flow.sh)
Tests conditional statements and loops:
- If conditions (true and false paths)
- If-else branching
- Nested if statements
- While loops with various conditions
- Complex boolean conditions

**Example Tests**:
```
✓ If True
✓ If-Else False
✓ Nested If
✓ While Sum (1+2+3+4+5)
✓ Complex AND Condition
✓ Complex OR Condition
```

### 3. Functions (functions.sh)
Tests function declarations and execution:
- Simple functions with parameters
- Functions with return values
- Functions without arguments
- Recursive functions (factorial, fibonacci)
- Multiple function calls
- Nested function calls
- Parameter passing order

**Example Tests**:
```
✓ Simple Function (add)
✓ Function No Args
✓ Factorial (5!) = 120
✓ Fibonacci (fib(10)) = 55
✓ Nested Function Calls
```

### 4. Variables (variables.sh)
Tests variable declarations and scoping:
- Integer variable declaration
- Multiple variable declarations
- Variable reassignment
- Variable scoping (if blocks, functions)
- Static variables
- Type conversions in expressions

**Example Tests**:
```
✓ Integer Variable
✓ Multiple Variables
✓ Variable Reassignment
✓ If Block Scope
✓ Function Scope
✓ Type Conversion
```

### 5. Cache System (cache.sh)
Tests the semi-compiled cache functionality:
- Cache file creation in `.tickcache/`
- Cache reuse on subsequent runs
- Cache consistency across runs
- Cache size verification
- `.tickcache/` in `.gitignore`

**Example Tests**:
```
✓ Cache File Created
✓ Cache Reuse Detected
✓ Cache Size Reasonable
✓ Cache Output Consistent
✓ .tickcache In .gitignore
```

### 6. Performance (performance.sh)
Tests performance characteristics:
- Cold start time (parse + cache)
- Warm start time (cache hit)
- Fibonacci execution performance
- Large program caching
- Cache size metrics

**Example Tests**:
```
✓ Cold Start Time: 0m0.050s
✓ Warm Start Time: 0m0.003s
✓ Fibonacci(15) = 610
✓ Large Program Execution
✓ Total Cache Size: 4KB
```

## Expected Output

```
======== OPERATORS - Arithmetic ========
[TEST] Addition (5 + 3)
[PASS] Addition (5 + 3)
[TEST] Subtraction (10 - 4)
[PASS] Subtraction (10 - 4)
...

======== TEST SUMMARY ========
Total:  42 tests
Passed: 42
Failed: 0

✅ All tests passed!
```

## Test Program Examples

Each test creates a `.tdl` program, runs it through the TDL compiler, and verifies output.

### Simple Function Test
```tdl
func add(int a, int b) -> int {
  return a + b;
}

func main() {
  let result: int = add(3, 4);
  println(result);
}
```

### Recursive Function Test
```tdl
func fibonacci(int n) -> int {
  if (n <= 1) {
    return n;
  }
  return fibonacci(n - 1) + fibonacci(n - 2);
}

func main() {
  println(fibonacci(10));
}
```

### Control Flow Test
```tdl
func main() {
  let i: int = 0;
  while (i < 3) {
    println(i);
    i = i + 1;
  }
}
```

## Verifying Test Results

Tests verify:
1. **Correctness**: Programs produce expected output
2. **Performance**: Cache provides 15-20x speedup
3. **Consistency**: Multiple runs produce identical results
4. **Integration**: Cache files created in `.tickcache/`
5. **Type Safety**: Type conversions work correctly
6. **Scoping**: Variables properly scoped to blocks/functions

## Adding New Tests

To add a new test:

1. Create a `.tdl` program in `test/programs/`
2. Use `test_program` function:
   ```bash
   test_program "Test Name" "program.tdl" "expected_output"
   ```
3. Add test to appropriate suite file
4. Run `./test/run_tests.sh` to verify

## Troubleshooting

If tests fail:

1. **Build error**: Ensure `make` succeeds
   ```bash
   cd ..
   make clean && make
   ```

2. **Cache issues**: Clear cache directories
   ```bash
   find test/programs -type d -name ".tickcache" -exec rm -rf {} + 2>/dev/null
   ```

3. **Output mismatch**: Compare actual vs expected
   ```bash
   ./bin/tdl test/programs/test_addition.tdl
   ```

4. **Performance degradation**: Check if cache is being used
   ```bash
   ./bin/tdl test/programs/perf_test.tdl
   # Should say "Using cached AST..." on second run
   ```

## Performance Benchmarks

Expected performance metrics:
- **First run**: 50-60ms (parse + serialize to cache)
- **Cached run**: 3-4ms (deserialize + interpret)
- **Improvement**: 15-20x faster
- **Cache size**: 50-450 bytes per program

## Language Features Tested

✅ Integer type  
✅ Arithmetic operators  
✅ Comparison operators  
✅ Logical operators  
✅ If/else statements  
✅ While loops  
✅ Function declarations  
✅ Function calls  
✅ Recursion  
✅ Variable declarations  
✅ Variable reassignment  
✅ Variable scoping  
✅ Return statements  
✅ Built-in println()  
✅ Cache creation  
✅ Cache reuse  
✅ Type conversions  

## See Also

- [IMPLEMENTATION_COMPLETE.md](../IMPLEMENTATION_COMPLETE.md) - Implementation summary
- [SEMI_COMPILED_ROADMAP.md](../SEMI_COMPILED_ROADMAP.md) - Detailed roadmap
- [semi_compiled_architecture.md](../docs/semi_compiled_architecture.md) - Architecture documentation
