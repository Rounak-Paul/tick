#!/bin/bash

# Arithmetic and Operator Tests

log_section "OPERATORS - Arithmetic"

# Create test programs
cat > "$TEST_DIR/programs/test_addition.tdl" << 'EOF'
func main() {
  let a: int = 5;
  let b: int = 3;
  println(a + b);
}
EOF

cat > "$TEST_DIR/programs/test_subtraction.tdl" << 'EOF'
func main() {
  let a: int = 10;
  let b: int = 4;
  println(a - b);
}
EOF

cat > "$TEST_DIR/programs/test_multiplication.tdl" << 'EOF'
func main() {
  let a: int = 6;
  let b: int = 7;
  println(a * b);
}
EOF

cat > "$TEST_DIR/programs/test_division.tdl" << 'EOF'
func main() {
  let a: int = 20;
  let b: int = 4;
  println(a / b);
}
EOF

cat > "$TEST_DIR/programs/test_modulo.tdl" << 'EOF'
func main() {
  let a: int = 17;
  let b: int = 5;
  println(a % b);
}
EOF

cat > "$TEST_DIR/programs/test_negation.tdl" << 'EOF'
func main() {
  let a: int = 42;
  println(-a);
}
EOF

# Run tests
test_program "Addition (5 + 3)" "$TEST_DIR/programs/test_addition.tdl" "8"
test_program "Subtraction (10 - 4)" "$TEST_DIR/programs/test_subtraction.tdl" "6"
test_program "Multiplication (6 * 7)" "$TEST_DIR/programs/test_multiplication.tdl" "42"
test_program "Division (20 / 4)" "$TEST_DIR/programs/test_division.tdl" "5"
test_program "Modulo (17 % 5)" "$TEST_DIR/programs/test_modulo.tdl" "2"
test_program "Negation (-42)" "$TEST_DIR/programs/test_negation.tdl" "-42"

log_section "OPERATORS - Comparison"

cat > "$TEST_DIR/programs/test_equal.tdl" << 'EOF'
func main() {
  let a: int = 5;
  let b: int = 5;
  println(a == b);
}
EOF

cat > "$TEST_DIR/programs/test_not_equal.tdl" << 'EOF'
func main() {
  let a: int = 5;
  let b: int = 3;
  println(a != b);
}
EOF

cat > "$TEST_DIR/programs/test_less_than.tdl" << 'EOF'
func main() {
  let a: int = 3;
  let b: int = 5;
  println(a < b);
}
EOF

cat > "$TEST_DIR/programs/test_greater_than.tdl" << 'EOF'
func main() {
  let a: int = 7;
  let b: int = 5;
  println(a > b);
}
EOF

cat > "$TEST_DIR/programs/test_less_equal.tdl" << 'EOF'
func main() {
  let a: int = 5;
  let b: int = 5;
  println(a <= b);
}
EOF

cat > "$TEST_DIR/programs/test_greater_equal.tdl" << 'EOF'
func main() {
  let a: int = 5;
  let b: int = 5;
  println(a >= b);
}
EOF

test_program "Equality (5 == 5)" "$TEST_DIR/programs/test_equal.tdl" "1"
test_program "Not Equal (5 != 3)" "$TEST_DIR/programs/test_not_equal.tdl" "1"
test_program "Less Than (3 < 5)" "$TEST_DIR/programs/test_less_than.tdl" "1"
test_program "Greater Than (7 > 5)" "$TEST_DIR/programs/test_greater_than.tdl" "1"
test_program "Less Or Equal (5 <= 5)" "$TEST_DIR/programs/test_less_equal.tdl" "1"
test_program "Greater Or Equal (5 >= 5)" "$TEST_DIR/programs/test_greater_equal.tdl" "1"

log_section "OPERATORS - Logical"

cat > "$TEST_DIR/programs/test_and.tdl" << 'EOF'
func main() {
  let a: int = 1;
  let b: int = 1;
  println(a && b);
}
EOF

cat > "$TEST_DIR/programs/test_or.tdl" << 'EOF'
func main() {
  let a: int = 0;
  let b: int = 1;
  println(a || b);
}
EOF

cat > "$TEST_DIR/programs/test_not.tdl" << 'EOF'
func main() {
  let a: int = 0;
  println(!a);
}
EOF

test_program "Logical AND (1 && 1)" "$TEST_DIR/programs/test_and.tdl" "1"
test_program "Logical OR (0 || 1)" "$TEST_DIR/programs/test_or.tdl" "1"
test_program "Logical NOT (!0)" "$TEST_DIR/programs/test_not.tdl" "1"

log_section "OPERATORS - Order of Operations"

cat > "$TEST_DIR/programs/test_precedence.tdl" << 'EOF'
func main() {
  let result: int = 2 + 3 * 4;
  println(result);
}
EOF

cat > "$TEST_DIR/programs/test_precedence_parens.tdl" << 'EOF'
func main() {
  let result: int = (2 + 3) * 4;
  println(result);
}
EOF

test_program "Operator Precedence (2 + 3 * 4)" "$TEST_DIR/programs/test_precedence.tdl" "14"
test_program "Parentheses ((2 + 3) * 4)" "$TEST_DIR/programs/test_precedence_parens.tdl" "20"
