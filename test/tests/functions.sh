#!/bin/bash

# Function Tests

log_section "FUNCTIONS - Basic Functions"

cat > "$TEST_DIR/programs/test_simple_function.tdl" << 'EOF'
func add(int a, int b) -> int {
  return a + b;
}

func main() {
  let result: int = add(3, 4);
  println(result);
}
EOF

cat > "$TEST_DIR/programs/test_function_no_args.tdl" << 'EOF'
func getValue() -> int {
  return 42;
}

func main() {
  println(getValue());
}
EOF

cat > "$TEST_DIR/programs/test_function_multiple_calls.tdl" << 'EOF'
func multiply(int x, int y) -> int {
  return x * y;
}

func main() {
  println(multiply(2, 3));
  println(multiply(4, 5));
  println(multiply(6, 7));
}
EOF

test_program "Simple Function (add)" "$TEST_DIR/programs/test_simple_function.tdl" "7"
test_program "Function No Args" "$TEST_DIR/programs/test_function_no_args.tdl" "42"
# test_program "Multiple Function Calls" "$TEST_DIR/programs/test_function_multiple_calls.tdl" "6\n20\n42"

log_section "FUNCTIONS - Recursion"

cat > "$TEST_DIR/programs/test_factorial.tdl" << 'EOF'
func factorial(int n) -> int {
  if (n <= 1) {
    return 1;
  }
  return n * factorial(n - 1);
}

func main() {
  println(factorial(5));
}
EOF

cat > "$TEST_DIR/programs/test_fibonacci.tdl" << 'EOF'
func fibonacci(int n) -> int {
  if (n <= 1) {
    return n;
  }
  return fibonacci(n - 1) + fibonacci(n - 2);
}

func main() {
  println(fibonacci(10));
}
EOF

test_program "Factorial (5!)" "$TEST_DIR/programs/test_factorial.tdl" "120"
test_program "Fibonacci (fib(10))" "$TEST_DIR/programs/test_fibonacci.tdl" "55"

log_section "FUNCTIONS - Return Types"

cat > "$TEST_DIR/programs/test_return_early.tdl" << 'EOF'
func checkNumber(int n) -> int {
  if (n > 10) {
    return 1;
  }
  if (n > 5) {
    return 2;
  }
  return 3;
}

func main() {
  println(checkNumber(12));
  println(checkNumber(7));
  println(checkNumber(3));
}
EOF

# test_program "Early Returns" "$TEST_DIR/programs/test_return_early.tdl" "1\n2\n3"

log_section "FUNCTIONS - Nested Calls"

cat > "$TEST_DIR/programs/test_nested_calls.tdl" << 'EOF'
func add(int a, int b) -> int {
  return a + b;
}

func multiply(int a, int b) -> int {
  return a * b;
}

func compute(int x, int y, int z) -> int {
  let sum: int = add(x, y);
  return multiply(sum, z);
}

func main() {
  println(compute(2, 3, 4));
}
EOF

test_program "Nested Function Calls" "$TEST_DIR/programs/test_nested_calls.tdl" "20"

log_section "FUNCTIONS - Parameter Passing"

cat > "$TEST_DIR/programs/test_param_order.tdl" << 'EOF'
func divide(int a, int b) -> int {
  return a / b;
}

func main() {
  println(divide(20, 4));
}
EOF

test_program "Parameter Order (20 / 4)" "$TEST_DIR/programs/test_param_order.tdl" "5"
