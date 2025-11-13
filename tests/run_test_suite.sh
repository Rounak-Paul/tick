#!/bin/bash

TICK_COMPILER="./build/tick"
TEST_DIR="tests/suite"
TEMP_DIR="/tmp/tick_tests"

mkdir -p "$TEMP_DIR"

echo "=========================================="
echo "    TICK LANGUAGE COMPREHENSIVE TEST SUITE"
echo "=========================================="
echo ""

total_pass=0
total_fail=0
test_count=0

for test_file in "$TEST_DIR"/*.tick; do
    test_count=$((test_count + 1))
    test_name=$(basename "$test_file" .tick)
    output="$TEMP_DIR/$test_name"
    
    echo "----------------------------------------"
    echo "Running: $test_name"
    echo "----------------------------------------"
    
    if "$TICK_COMPILER" "$test_file" -o "$output" 2>&1 | grep -q "Success"; then
        "$output"
        exit_code=$?
        
        if [ $exit_code -eq 0 ]; then
            echo ""
            echo "✓ $test_name: ALL TESTS PASSED"
            total_pass=$((total_pass + 1))
        else
            echo ""
            echo "✗ $test_name: $exit_code TEST(S) FAILED"
            total_fail=$((total_fail + 1))
        fi
    else
        echo "✗ $test_name: COMPILATION FAILED"
        total_fail=$((total_fail + 1))
    fi
    
    echo ""
done

echo "=========================================="
echo "    TEST SUITE SUMMARY"
echo "=========================================="
echo "Total test files: $test_count"
echo "Passed: $total_pass"
echo "Failed: $total_fail"
echo ""

if [ $total_fail -eq 0 ]; then
    echo "🎉 ALL TESTS PASSED!"
    exit 0
else
    echo "❌ SOME TESTS FAILED"
    exit 1
fi
