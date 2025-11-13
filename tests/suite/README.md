# Tick Language Comprehensive Test Suite

This directory contains a comprehensive test suite for the Tick programming language, covering all implemented features.

## Running the Tests

```bash
./tests/run_test_suite.sh
```

## Test Coverage

### Basic Language Features
- **test_basic_types.tick** - Basic type system (int, float, bool)
- **test_arithmetic.tick** - Arithmetic operations (+, -, *, /, %)
- **test_comparison.tick** - Comparison operators (==, !=, <, >, <=, >=)
- **test_logical.tick** - Logical operators (&&, ||, !)
- **test_compound_ops.tick** - Compound assignment operators (+=, -=, *=, /=, %=)
- **test_increment.tick** - Prefix increment/decrement (++x, --x)
- **test_const.tick** - Constant variables (global and local)

### Data Structures
- **test_arrays.tick** - Array literals, indexing, all types, nested access
- **test_classes.tick** - Classes with constructors, methods, member access, this keyword

### Control Flow
- **test_control_flow.tick** - if/else, while loops, break, continue, nested loops
- **test_functions.tick** - Function calls, recursion, return values, void functions

### Advanced Features
- **test_scoping.tick** - Variable scoping, shadowing, global/local variables
- **test_complex_expressions.tick** - Operator precedence, parentheses, complex boolean logic

### Concurrent Features
- **test_events.tick** - Events and process execution
- **test_signals_basic.tick** - Signals with int and bool types, queuing
- **test_signals_arrays.tick** - Arrays of signals (signal[n])
- **test_signals_array_types.tick** - Arrays as signal types (signal : type[])
- **test_signals_classes.tick** - Classes as signal types

### Real-World Tests
- **test_fibonacci.tick** - Fibonacci (recursive and iterative implementations)
- **test_edge_cases.tick** - Edge cases, negative numbers, large numbers, empty arrays

## Test Results (Current)

**Status:** 19/20 tests passing ✅

**Known Issues:**
- `test_classes.tick` - Fails due to semantic analyzer limitation where methods with the same name across different classes are incorrectly flagged as duplicates. This is a compiler bug, not a language limitation. Tests pass when method names are unique across all classes.

## Test Statistics

- **Total test files:** 20
- **Total test cases:** ~120+
- **Lines of test code:** ~1,500+
- **Features covered:** All implemented language features

## Writing New Tests

Each test file follows this pattern:

```tick
func main() : int {
    var pass : int = 0;
    var fail : int = 0;
    
    println("=== Test Name ===");
    
    // Test case
    if (condition) {
        println("PASS: test description");
        pass = pass + 1;
    } else {
        println("FAIL: test description");
        fail = fail + 1;
    }
    
    println("Passed: ");
    println(pass);
    println("Failed: ");
    println(fail);
    
    return fail;  // Exit with number of failures
}
```

## Features Tested

### ✅ Implemented and Tested
- [x] Basic types (int, float, bool)
- [x] Arithmetic operators
- [x] Comparison operators  
- [x] Logical operators
- [x] Compound assignments
- [x] Prefix increment/decrement
- [x] Const variables
- [x] Arrays (literals, indexing, all types)
- [x] Classes (C++-style constructors, methods, member access, this)
- [x] Control flow (if/else, while, break, continue)
- [x] Functions (calls, recursion, return values)
- [x] Scoping (global, local, shadowing)
- [x] Events and processes
- [x] Generic signals (int, bool, arrays, classes)
- [x] Signal arrays
- [x] Complex expressions

### ⚠️  Limitations
- Float signals: Casting through intptr_t loses precision; use int or pointer types
- Method names: Must be unique across all classes (semantic analyzer limitation)

### 🚧 Not Yet Implemented
- String operations (concatenation, comparison, length, substr)
- Type casting (int(), float(), bool(), string())
- Postfix increment/decrement (x++, x--)
- Import/module system

## Notes

- Tests use signals with `done.emit(1)` and `done.recv()` for synchronization with concurrent processes
- All tests are self-contained and can run independently
- Exit code 0 = all tests passed, non-zero = number of failures
