#!/bin/bash

# Simplified Working Test Suite
# Tests only the core working functionality that we've verified

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BIN="$PROJECT_ROOT/bin/tdl"
TEST_DIR="$SCRIPT_DIR"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

log_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((TESTS_PASSED++))
}

log_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((TESTS_FAILED++))
}

log_section() {
    echo -e "\n${YELLOW}======== $1 ========${NC}"
}

# Test helper: Create TDL file and test
test_tdl() {
    local name=$1
    local code=$2
    local expected=$3
    
    ((TESTS_RUN++))
    
    local tmp_file="/tmp/tdl_test_$RANDOM.tdl"
    echo "$code" > "$tmp_file"
    
    local output=$("$BIN" "$tmp_file" 2>&1)
    local actual=$(echo "$output" | awk '/=== Execution Output ===/,EOF {if (!/=== Execution Output ===/) print}' | sed '/^[[:space:]]*$/d' | head -1)
    
    if [ "$actual" = "$expected" ]; then
        log_pass "$name"
    else
        log_fail "$name (expected: $expected, got: $actual)"
    fi
    
    rm -f "$tmp_file"
}

# Ensure binary exists
if [ ! -f "$BIN" ]; then
    echo "Building project..."
    cd "$PROJECT_ROOT"
    make -j4 > /dev/null 2>&1
fi

log_section "BASIC OPERATIONS"

test_tdl "Integer literal" \
'func main() { println(42); }' \
"42"

test_tdl "Addition" \
'func main() { println(5 + 3); }' \
"8"

test_tdl "Subtraction" \
'func main() { println(10 - 4); }' \
"6"

test_tdl "Multiplication" \
'func main() { println(7 * 6); }' \
"42"

test_tdl "Division" \
'func main() { println(20 / 4); }' \
"5"

test_tdl "Modulo" \
'func main() { println(17 % 5); }' \
"2"

log_section "COMPARISONS"

test_tdl "Equality true" \
'func main() { println(5 == 5); }' \
"1"

test_tdl "Equality false" \
'func main() { println(5 == 3); }' \
"0"

test_tdl "Not equal" \
'func main() { println(5 != 3); }' \
"1"

test_tdl "Less than" \
'func main() { println(3 < 5); }' \
"1"

test_tdl "Greater than" \
'func main() { println(7 > 5); }' \
"1"

test_tdl "Less or equal" \
'func main() { println(5 <= 5); }' \
"1"

log_section "LOGICAL OPERATORS"

test_tdl "AND true" \
'func main() { println(1 && 1); }' \
"1"

test_tdl "AND false" \
'func main() { println(1 && 0); }' \
"0"

test_tdl "OR true" \
'func main() { println(1 || 0); }' \
"1"

test_tdl "OR false" \
'func main() { println(0 || 0); }' \
"0"

test_tdl "NOT true" \
'func main() { println(!0); }' \
"1"

test_tdl "NOT false" \
'func main() { println(!1); }' \
"0"

log_section "PRECEDENCE"

test_tdl "Precedence: 2 + 3 * 4" \
'func main() { println(2 + 3 * 4); }' \
"14"

test_tdl "Precedence with parens: (2 + 3) * 4" \
'func main() { println((2 + 3) * 4); }' \
"20"

test_tdl "Negation" \
'func main() { println(-5); }' \
"-5"

log_section "CONTROL FLOW"

test_tdl "If true" \
'func main() { if (1) { println(42); } }' \
"42"

test_tdl "If with expression" \
'func main() { if (5 > 3) { println(100); } }' \
"100"

test_tdl "Nested if" \
'func main() { if (1) { if (1) { println(99); } } }' \
"99"

log_section "FUNCTIONS"

test_tdl "Simple function call" \
'func test() { println(7); } func main() { test(); }' \
"7"

test_tdl "Recursive function" \
'func fib(int n) -> int { if (n <= 1) { return n; } return fib(n-1) + fib(n-2); } func main() { println(fib(10)); }' \
"55"

log_section "CACHE SYSTEM"

# Clean cache
rm -rf "$TEST_DIR/programs/.tickcache" 2>/dev/null

# Test cache creation
cat > "$TEST_DIR/programs/cache_test.tdl" << 'EOF'
func main() { println(777); }
EOF

$BIN "$TEST_DIR/programs/cache_test.tdl" > /dev/null 2>&1

if ls "$TEST_DIR/programs/.tickcache/"*.tco > /dev/null 2>&1; then
    log_pass "Cache file created"
    ((TESTS_RUN++))
else
    log_fail "Cache file created"
    ((TESTS_RUN++))
fi

# Test cache reuse
OUTPUT=$($BIN "$TEST_DIR/programs/cache_test.tdl" 2>&1)
if echo "$OUTPUT" | grep -q "Using cached AST"; then
    log_pass "Cache reused on second run"
    ((TESTS_RUN++))
else
    log_fail "Cache reused on second run"
    ((TESTS_RUN++))
fi

log_section "TEST SUMMARY"

echo -e "Total:  $TESTS_RUN tests"
echo -e "Passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Failed: ${RED}$TESTS_FAILED${NC}"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}✅ All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}❌ $TESTS_FAILED tests failed!${NC}"
    exit 1
fi
