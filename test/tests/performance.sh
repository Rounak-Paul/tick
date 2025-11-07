#!/bin/bash

# Performance Tests

log_section "PERFORMANCE - Cold Start (Parse + Cache)"

cat > "$TEST_DIR/programs/perf_test.tdl" << 'EOF'
func fibonacci(int n) -> int {
  if (n <= 1) {
    return n;
  }
  return fibonacci(n - 1) + fibonacci(n - 2);
}

func main() {
  println(fibonacci(15));
}
EOF

# Remove cache to force cold start
rm -rf "$TEST_DIR/programs/.tickcache" 2>/dev/null

# Time cold start
COLD_START=$( { time "$BIN" "$TEST_DIR/programs/perf_test.tdl" > /dev/null 2>&1; } 2>&1 | grep real | awk '{print $2}')

((TESTS_RUN++))
log_pass "Cold Start Time: $COLD_START"

log_section "PERFORMANCE - Warm Start (Cache Hit)"

# Time warm start (using cache)
WARM_START=$( { time "$BIN" "$TEST_DIR/programs/perf_test.tdl" > /dev/null 2>&1; } 2>&1 | grep real | awk '{print $2}')

((TESTS_RUN++))
log_pass "Warm Start Time: $WARM_START"

log_section "PERFORMANCE - Fibonacci Execution"

FIBO_OUTPUT=$("$BIN" "$TEST_DIR/programs/perf_test.tdl" 2>&1 | grep "=== Execution Output ===" -A 1 | tail -1)

if [ "$FIBO_OUTPUT" = "610" ]; then
    ((TESTS_RUN++))
    log_pass "Fibonacci(15) = 610"
else
    ((TESTS_RUN++))
    ((TESTS_FAILED++))
    log_fail "Fibonacci(15) Correct Output (got $FIBO_OUTPUT)"
fi

log_section "PERFORMANCE - Large Program Caching"

# Create a larger program
cat > "$TEST_DIR/programs/large_program.tdl" << 'EOF'
func sum(int n) -> int {
  let result: int = 0;
  let i: int = 0;
  while (i <= n) {
    result = result + i;
    i = i + 1;
  }
  return result;
}

func factorial(int n) -> int {
  if (n <= 1) {
    return 1;
  }
  return n * factorial(n - 1);
}

func power(int base, int exp) -> int {
  if (exp == 0) {
    return 1;
  }
  return base * power(base, exp - 1);
}

func main() {
  let s: int = sum(10);
  let f: int = factorial(5);
  let p: int = power(2, 5);
  println(s);
  println(f);
  println(p);
}
EOF

# Remove cache
rm -rf "$TEST_DIR/programs/.tickcache" 2>/dev/null

# Check if program runs
OUTPUT=$("$BIN" "$TEST_DIR/programs/large_program.tdl" 2>&1)

if echo "$OUTPUT" | grep -q "55" && echo "$OUTPUT" | grep -q "120"; then
    ((TESTS_RUN++))
    log_pass "Large Program Execution"
else
    ((TESTS_RUN++))
    ((TESTS_FAILED++))
    log_fail "Large Program Execution"
fi

# Check cache size for large program
if [ -f "$TEST_DIR/programs/.tickcache/"*.tco ]; then
    CACHE_SIZES=$(du -sb "$TEST_DIR/programs/.tickcache/"*.tco 2>/dev/null | awk '{sum+=$1} END {print sum}')
    ((TESTS_RUN++))
    log_pass "Total Cache Size: $((CACHE_SIZES / 1024))KB"
fi
