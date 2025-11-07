# TDL Language Test Suite

Comprehensive test suite for the TDL (Time-Deterministic Language) semi-compiled system.

## Overview

This test suite thoroughly tests all language features including:
- **Arithmetic Operators**: +, -, *, /, %, unary negation
- **Comparison Operators**: ==, !=, <, >, <=, >=
- **Logical Operators**: &&, ||, !
- **Control Flow**: if/else, while loops, nested conditions
- **Functions**: declarations, calls, recursion, return values
- **Variables**: declaration, reassignment, scoping, static variables
- **Type System**: int, double, bool, string with auto-conversion
- **Cache System**: creation, reuse, consistency, performance

## Running Tests

### Run All Tests
```bash
./test/run_tests.sh
```

### Run Specific Test Suite
```bash
bash test/tests/operators.sh
bash test/tests/control_flow.sh
bash test/tests/functions.sh
bash test/tests/variables.sh
bash test/tests/cache.sh
bash test/tests/performance.sh
```

## Test Structure

```
test/
├── run_tests.sh          # Main test runner
├── tests/                # Test suites
│   ├── operators.sh      # Arithmetic, comparison, logical operators
│   ├── control_flow.sh   # If/else, while loops
│   ├── functions.sh      # Function definitions, calls, recursion
│   ├── variables.sh      # Variable declarations, scoping
│   ├── cache.sh          # Cache creation, reuse, consistency
│   └── performance.sh    # Performance benchmarks
└── programs/             # Generated test programs
    ├── test_*.tdl        # Individual test programs
    └── .tickcache/       # Cache files generated during tests
```

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
