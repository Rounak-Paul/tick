#!/bin/bash

# Control Flow Tests

log_section "CONTROL FLOW - If/Else"

cat > "$TEST_DIR/programs/test_if_true.tdl" << 'EOF'
func main() {
  let a: int = 5;
  if (a > 3) {
    println(1);
  }
}
EOF

cat > "$TEST_DIR/programs/test_if_false.tdl" << 'EOF'
func main() {
  let a: int = 2;
  if (a > 3) {
    println(0);
  } else {
    println(1);
  }
}
EOF

cat > "$TEST_DIR/programs/test_if_nested.tdl" << 'EOF'
func main() {
  let a: int = 5;
  let b: int = 10;
  if (a > 3) {
    if (b > 8) {
      println(1);
    }
  }
}
EOF

test_program "If True" "$TEST_DIR/programs/test_if_true.tdl" "1"
test_program "If-Else False" "$TEST_DIR/programs/test_if_false.tdl" "1"
test_program "Nested If" "$TEST_DIR/programs/test_if_nested.tdl" "1"

log_section "CONTROL FLOW - While Loop"

cat > "$TEST_DIR/programs/test_while_simple.tdl" << 'EOF'
func main() {
  let i: int = 0;
  while (i < 3) {
    println(i);
    i = i + 1;
  }
}
EOF

cat > "$TEST_DIR/programs/test_while_sum.tdl" << 'EOF'
func main() {
  let i: int = 1;
  let sum: int = 0;
  while (i <= 5) {
    sum = sum + i;
    i = i + 1;
  }
  println(sum);
}
EOF

# Note: Output has newlines, need special handling
# test_program "While Loop" "$TEST_DIR/programs/test_while_simple.tdl" "0\n1\n2"
test_program "While Sum (1+2+3+4+5)" "$TEST_DIR/programs/test_while_sum.tdl" "15"

log_section "CONTROL FLOW - Complex Conditions"

cat > "$TEST_DIR/programs/test_complex_condition.tdl" << 'EOF'
func main() {
  let a: int = 5;
  let b: int = 10;
  if (a > 3 && b < 15) {
    println(1);
  } else {
    println(0);
  }
}
EOF

cat > "$TEST_DIR/programs/test_or_condition.tdl" << 'EOF'
func main() {
  let a: int = 1;
  let b: int = 5;
  if (a > 10 || b < 10) {
    println(1);
  } else {
    println(0);
  }
}
EOF

test_program "Complex AND Condition" "$TEST_DIR/programs/test_complex_condition.tdl" "1"
test_program "Complex OR Condition" "$TEST_DIR/programs/test_or_condition.tdl" "1"
