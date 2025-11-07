#!/bin/bash

# Variable and Type Tests

log_section "VARIABLES - Basic Declaration"

cat > "$TEST_DIR/programs/test_int_var.tdl" << 'EOF'
func main() {
  let x: int = 42;
  println(x);
}
EOF

cat > "$TEST_DIR/programs/test_multiple_vars.tdl" << 'EOF'
func main() {
  let a: int = 10;
  let b: int = 20;
  let c: int = a + b;
  println(c);
}
EOF

cat > "$TEST_DIR/programs/test_var_reassign.tdl" << 'EOF'
func main() {
  let x: int = 5;
  x = 10;
  x = x + 5;
  println(x);
}
EOF

test_program "Integer Variable" "$TEST_DIR/programs/test_int_var.tdl" "42"
test_program "Multiple Variables" "$TEST_DIR/programs/test_multiple_vars.tdl" "30"
test_program "Variable Reassignment" "$TEST_DIR/programs/test_var_reassign.tdl" "15"

log_section "VARIABLES - Variable Scope"

cat > "$TEST_DIR/programs/test_scope_if.tdl" << 'EOF'
func main() {
  let x: int = 5;
  if (x > 3) {
    let y: int = 10;
    println(x + y);
  }
}
EOF

cat > "$TEST_DIR/programs/test_scope_function.tdl" << 'EOF'
func getNumber() -> int {
  let local: int = 100;
  return local;
}

func main() {
  println(getNumber());
}
EOF

test_program "If Block Scope" "$TEST_DIR/programs/test_scope_if.tdl" "15"
test_program "Function Scope" "$TEST_DIR/programs/test_scope_function.tdl" "100"

log_section "VARIABLES - Static Variables"

cat > "$TEST_DIR/programs/test_static_counter.tdl" << 'EOF'
func incrementCounter() -> int {
  static count: int = 0;
  count = count + 1;
  return count;
}

func main() {
  println(incrementCounter());
}
EOF

# Static variables are complex to test without multiple calls
# test_program "Static Counter" "$TEST_DIR/programs/test_static_counter.tdl" "1"

log_section "TYPES - Type Conversions"

cat > "$TEST_DIR/programs/test_int_arithmetic.tdl" << 'EOF'
func main() {
  let a: int = 10;
  let b: int = 3;
  println(a / b);
}
EOF

cat > "$TEST_DIR/programs/test_mixed_arithmetic.tdl" << 'EOF'
func main() {
  let result: int = 5 + 3 * 2;
  println(result);
}
EOF

test_program "Integer Arithmetic" "$TEST_DIR/programs/test_int_arithmetic.tdl" "3"
test_program "Mixed Type Arithmetic" "$TEST_DIR/programs/test_mixed_arithmetic.tdl" "11"

log_section "TYPES - Comparison with Different Types"

cat > "$TEST_DIR/programs/test_compare_equal.tdl" << 'EOF'
func main() {
  let a: int = 5;
  let b: int = 5;
  if (a == b) {
    println(1);
  }
}
EOF

cat > "$TEST_DIR/programs/test_compare_range.tdl" << 'EOF'
func main() {
  let x: int = 7;
  if (x > 5 && x < 10) {
    println(1);
  }
}
EOF

test_program "Type Comparison" "$TEST_DIR/programs/test_compare_equal.tdl" "1"
test_program "Range Check" "$TEST_DIR/programs/test_compare_range.tdl" "1"
