#!/bin/bash

# Cache System Tests

log_section "CACHE - Cache Creation"

cat > "$TEST_DIR/programs/cache_test.tdl" << 'EOF'
func main() {
  println(123);
}
EOF

# Run once to create cache
"$BIN" "$TEST_DIR/programs/cache_test.tdl" > /dev/null 2>&1

# Check if cache file exists
if ls "$TEST_DIR/programs/.tickcache/"*.tco > /dev/null 2>&1; then
    ((TESTS_RUN++))
    log_pass "Cache File Created"
else
    ((TESTS_RUN++))
    ((TESTS_FAILED++))
    log_fail "Cache File Created"
fi

log_section "CACHE - Cache Reuse"

# Run again and check output mentions cache
OUTPUT=$("$BIN" "$TEST_DIR/programs/cache_test.tdl" 2>&1)

if echo "$OUTPUT" | grep -q "Using cached AST"; then
    ((TESTS_RUN++))
    log_pass "Cache Reuse Detected"
else
    ((TESTS_RUN++))
    ((TESTS_FAILED++))
    log_fail "Cache Reuse Detected"
    echo "  Output: $OUTPUT"
fi

log_section "CACHE - Cache Size"

if [ -f "$TEST_DIR/programs/.tickcache/"*.tco ]; then
    CACHE_SIZE=$(du -sb "$TEST_DIR/programs/.tickcache/"*.tco 2>/dev/null | awk '{print $1}')
    if [ "$CACHE_SIZE" -lt 10000 ]; then
        ((TESTS_RUN++))
        log_pass "Cache Size Reasonable (${CACHE_SIZE} bytes)"
    else
        ((TESTS_RUN++))
        ((TESTS_FAILED++))
        log_fail "Cache Size Too Large (${CACHE_SIZE} bytes)"
    fi
fi

log_section "CACHE - Cache Consistency"

# Run multiple times and verify consistent output
OUTPUT1=$("$BIN" "$TEST_DIR/programs/cache_test.tdl" 2>&1 | grep "=== Execution Output ===" -A 1 | tail -1)
OUTPUT2=$("$BIN" "$TEST_DIR/programs/cache_test.tdl" 2>&1 | grep "=== Execution Output ===" -A 1 | tail -1)

if [ "$OUTPUT1" = "$OUTPUT2" ]; then
    ((TESTS_RUN++))
    log_pass "Cache Output Consistent"
else
    ((TESTS_RUN++))
    ((TESTS_FAILED++))
    log_fail "Cache Output Consistent"
    echo "  Run 1: $OUTPUT1"
    echo "  Run 2: $OUTPUT2"
fi

log_section "CACHE - .tickcache Exclusion"

if grep -q ".tickcache" "$PROJECT_ROOT/.gitignore"; then
    ((TESTS_RUN++))
    log_pass ".tickcache In .gitignore"
else
    ((TESTS_RUN++))
    ((TESTS_FAILED++))
    log_fail ".tickcache In .gitignore"
fi
