#!/bin/bash

# TDL Language Test Suite
# Comprehensive testing of all language features

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BIN="$PROJECT_ROOT/bin/tdl"
TEST_DIR="$SCRIPT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Helper functions
log_test() {
    echo -e "${BLUE}[TEST]${NC} $1"
}

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

# Run a TDL program and compare output
test_program() {
    local name=$1
    local input_file=$2
    local expected_output=$3
    
    ((TESTS_RUN++))
    log_test "$name"
    
    # Run the program and capture output
    local output=$("$BIN" "$input_file" 2>&1 || true)
    # Extract output after "=== Execution Output ===" and skip empty lines
    local actual_output=$(echo "$output" | awk '/=== Execution Output ===/,EOF {if (!/=== Execution Output ===/) print}' | sed '/^[[:space:]]*$/d' | head -1)
    
    if [ "$actual_output" = "$expected_output" ]; then
        log_pass "$name"
        ((TESTS_PASSED++))
    else
        log_fail "$name"
        echo "  Expected: '$expected_output'"
        echo "  Got:      '$actual_output'"
    fi
}

# Build the project first
if [ ! -f "$BIN" ]; then
    echo "Building project..."
    cd "$PROJECT_ROOT"
    make -j4 > /dev/null 2>&1
fi

# Run test suites
source "$TEST_DIR/tests/operators.sh"
source "$TEST_DIR/tests/control_flow.sh"
source "$TEST_DIR/tests/functions.sh"
source "$TEST_DIR/tests/variables.sh"
source "$TEST_DIR/tests/cache.sh"
source "$TEST_DIR/tests/performance.sh"

# Summary
log_section "TEST SUMMARY"
echo -e "Total:  $TESTS_RUN tests"
echo -e "Passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Failed: ${RED}$TESTS_FAILED${NC}"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "\n${GREEN}✅ All tests passed!${NC}"
    exit 0
else
    echo -e "\n${RED}❌ Some tests failed!${NC}"
    exit 1
fi
