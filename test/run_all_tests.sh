#!/bin/bash

# TDL Test Suite - Automated Testing
# This script runs all TDL examples and verifies output

set -e

TOTAL=0
PASSED=0
FAILED=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test function
run_test() {
  local name=$1
  local file=$2
  local expected=$3
  
  TOTAL=$((TOTAL + 1))
  
  echo -n "Testing $name... "
  
  if output=$(/Users/duke/Code/tick/build/bin/tdl "$file" 2>&1 | grep "=== Execution Output ===" -A 100 | tail -n +2); then
    output=$(echo "$output" | head -1)
    if [ "$output" = "$expected" ]; then
      echo -e "${GREEN}✓ PASS${NC}"
      PASSED=$((PASSED + 1))
    else
      echo -e "${RED}✗ FAIL${NC}"
      echo "  Expected: $expected"
      echo "  Got: $output"
      FAILED=$((FAILED + 1))
    fi
  else
    echo -e "${RED}✗ ERROR${NC}"
    FAILED=$((FAILED + 1))
  fi
}

echo "=== TDL Test Suite ==="
echo ""

# Basics
echo -e "${YELLOW}Basics:${NC}"
run_test "01_hello_world" "/Users/duke/Code/tick/examples/basics/01_hello_world.tdl" "42"
run_test "02_variables" "/Users/duke/Code/tick/examples/basics/02_variables.tdl" "30"
run_test "03_if_else" "/Users/duke/Code/tick/examples/basics/03_if_else.tdl" "15"
run_test "04_while_loop" "/Users/duke/Code/tick/examples/basics/04_while_loop.tdl" "0"
run_test "05_recursion" "/Users/duke/Code/tick/examples/basics/05_recursion.tdl" "120"
run_test "06_static_variables" "/Users/duke/Code/tick/examples/basics/06_static_variables.tdl" "1"

echo ""
echo -e "${YELLOW}Intermediate:${NC}"
run_test "01_fibonacci" "/Users/duke/Code/tick/examples/intermediate/01_fibonacci.tdl" "55"
run_test "02_parallel_basic" "/Users/duke/Code/tick/examples/intermediate/02_parallel_basic.tdl" "210"
run_test "03_parallel_map_reduce" "/Users/duke/Code/tick/examples/intermediate/03_parallel_map_reduce.tdl" "54"
run_test "04_deterministic_accumulation" "/Users/duke/Code/tick/examples/intermediate/04_deterministic_accumulation.tdl" "10"
run_test "05_pipeline" "/Users/duke/Code/tick/examples/intermediate/05_pipeline.tdl" "60"

echo ""
echo -e "${YELLOW}Advanced:${NC}"
run_test "01_large_parallel_sum" "/Users/duke/Code/tick/examples/advanced/01_large_parallel_sum.tdl" "136"
run_test "02_mixed_parallelism" "/Users/duke/Code/tick/examples/advanced/02_mixed_parallelism.tdl" "66"
run_test "03_recursive_computation" "/Users/duke/Code/tick/examples/advanced/03_recursive_computation.tdl" "15"
run_test "04_deterministic_state" "/Users/duke/Code/tick/examples/advanced/04_deterministic_state.tdl" "3"

echo ""
echo "=== Test Results ==="
echo "Total: $TOTAL"
echo -e "Passed: ${GREEN}$PASSED${NC}"
echo -e "Failed: ${RED}$FAILED${NC}"

if [ $FAILED -eq 0 ]; then
  echo -e "${GREEN}✓ All tests passed!${NC}"
  exit 0
else
  echo -e "${RED}✗ Some tests failed${NC}"
  exit 1
fi
