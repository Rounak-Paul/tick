#!/bin/bash

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║                  TICK COMPREHENSIVE TEST SUITE                 ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

TEST_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$TEST_DIR/../build"
TICK_BIN="$BUILD_DIR/tick"
TEST_BUILD_DIR="$BUILD_DIR/tests"

cd "$TEST_DIR/.."

if [ ! -f "$TICK_BIN" ]; then
    echo "Building tick compiler..."
    ./build.sh
    if [ $? -ne 0 ]; then
        echo "✗ Build failed"
        exit 1
    fi
fi

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║                    UNIT TESTS                                  ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

cd "$BUILD_DIR"

TOTAL_PASSED=0
TOTAL_FAILED=0

if [ -f "$TEST_BUILD_DIR/test_core" ]; then
    echo "Running Core Data Structure Tests..."
    "$TEST_BUILD_DIR/test_core"
    RESULT=$?
    if [ $RESULT -eq 0 ]; then
        echo "✓ Core tests passed"
        TOTAL_PASSED=$((TOTAL_PASSED + 1))
    else
        echo "✗ Core tests failed"
        TOTAL_FAILED=$((TOTAL_FAILED + 1))
    fi
    echo ""
fi

if [ -f "$TEST_BUILD_DIR/test_compiler" ]; then
    echo "Running Compiler Tests..."
    "$TEST_BUILD_DIR/test_compiler"
    RESULT=$?
    if [ $RESULT -eq 0 ]; then
        echo "✓ Compiler tests passed"
        TOTAL_PASSED=$((TOTAL_PASSED + 1))
    else
        echo "✗ Compiler tests failed"
        TOTAL_FAILED=$((TOTAL_FAILED + 1))
    fi
    echo ""
fi

if [ -f "$TEST_BUILD_DIR/test_runtime" ]; then
    echo "Running Runtime Tests..."
    "$TEST_BUILD_DIR/test_runtime"
    RESULT=$?
    if [ $RESULT -eq 0 ]; then
        echo "✓ Runtime tests passed"
        TOTAL_PASSED=$((TOTAL_PASSED + 1))
    else
        echo "✗ Runtime tests failed"
        TOTAL_FAILED=$((TOTAL_FAILED + 1))
    fi
    echo ""
fi

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║                  INTEGRATION TESTS                             ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

cd "$TEST_DIR/.."

for EXAMPLE in examples/*.tick; do
    BASENAME=$(basename "$EXAMPLE")
    echo -n "Testing $BASENAME... "
    "$TICK_BIN" "$EXAMPLE" > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo "✓"
        TOTAL_PASSED=$((TOTAL_PASSED + 1))
    else
        echo "✗"
        TOTAL_FAILED=$((TOTAL_FAILED + 1))
    fi
done

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║                  PERFORMANCE BENCHMARKS                        ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

run_benchmark() {
    local FILE=$1
    local NAME=$2
    local ITERATIONS=$3
    
    echo "=== $NAME ==="
    
    local TOTAL_TIME=0
    local MIN_TIME=999999
    local MAX_TIME=0
    
    for i in $(seq 1 $ITERATIONS); do
        START=$(date +%s%N)
        "$TICK_BIN" "$FILE" > /dev/null 2>&1
        END=$(date +%s%N)
        
        ELAPSED=$((($END - $START) / 1000000))
        TOTAL_TIME=$(($TOTAL_TIME + $ELAPSED))
        
        if [ $ELAPSED -lt $MIN_TIME ]; then
            MIN_TIME=$ELAPSED
        fi
        if [ $ELAPSED -gt $MAX_TIME ]; then
            MAX_TIME=$ELAPSED
        fi
    done
    
    AVG_TIME=$(($TOTAL_TIME / $ITERATIONS))
    
    echo "  Iterations: $ITERATIONS"
    echo "  Average:    ${AVG_TIME}ms"
    echo "  Min:        ${MIN_TIME}ms"
    echo "  Max:        ${MAX_TIME}ms"
    echo ""
}

if [ -f "tests/perf_sequential.tick" ]; then
    run_benchmark "tests/perf_sequential.tick" "Sequential Computation (Fibonacci)" 10
fi

if [ -f "tests/perf_parallel.tick" ]; then
    run_benchmark "tests/perf_parallel.tick" "Parallel Execution (4 Workers)" 10
fi

if [ -f "tests/perf_pipeline.tick" ]; then
    run_benchmark "tests/perf_pipeline.tick" "Pipeline Communication (5 Stages)" 10
fi

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║                  MEMORY & RESOURCE STATS                       ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

BINARY_SIZE=$(ls -lh "$TICK_BIN" | awk '{print $5}')
echo "Binary Size:     $BINARY_SIZE"

SOURCE_FILES=$(find src -name "*.cpp" -o -name "*.h" | wc -l | tr -d ' ')
echo "Source Files:    $SOURCE_FILES"

LINES_OF_CODE=$(find src -name "*.cpp" -o -name "*.h" | xargs wc -l | tail -1 | awk '{print $1}')
echo "Lines of Code:   $LINES_OF_CODE"

CPU_CORES=$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo "Unknown")
echo "CPU Cores:       $CPU_CORES"

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║                    FINAL RESULTS                               ║"
echo "╠═══════════════════════════════════════════════════════════════╣"
echo "║   Passed: $TOTAL_PASSED"
echo "║   Failed: $TOTAL_FAILED"
echo "║   Total:  $((TOTAL_PASSED + TOTAL_FAILED))"
echo "╚═══════════════════════════════════════════════════════════════╝"

if [ $TOTAL_FAILED -eq 0 ]; then
    echo ""
    echo "🎉 ALL TESTS PASSED! 🎉"
    exit 0
else
    echo ""
    echo "⚠️  SOME TESTS FAILED"
    exit 1
fi
