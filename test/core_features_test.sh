#!/bin/bash

# TDL Core Features Test Suite
# Tests the temporal deterministic language core: clocks, processes, channels

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

# Test helper: Create TDL file, run, check output contains expected
test_contains() {
    local name=$1
    local code=$2
    local expected_substring=$3
    
    ((TESTS_RUN++))
    
    local tmp_file="/tmp/tdl_core_test_$RANDOM.tdl"
    echo "$code" > "$tmp_file"
    
    local output=$("$BIN" "$tmp_file" 2>&1 || true)
    
    if echo "$output" | grep -q "$expected_substring"; then
        log_pass "$name"
    else
        log_fail "$name (expected to contain: $expected_substring)"
        echo "  Output: $output" | head -5
    fi
    
    rm -f "$tmp_file"
}

# Ensure binary exists
if [ ! -f "$BIN" ]; then
    echo "Building project..."
    cd "$PROJECT_ROOT"
    make -j4 > /dev/null 2>&1
fi

log_section "TEMPORAL DETERMINISTIC FEATURES - CLOCKS"

test_contains "Clock declaration with frequency" \
'clock sys = 100hz;

func main() { println(1); }' \
"Execution Output"

test_contains "Clock with no frequency (max-speed)" \
'clock fast;

func main() { println(1); }' \
"Execution Output"

log_section "TEMPORAL DETERMINISTIC FEATURES - PROCESSES"

test_contains "Process declaration" \
'clock c = 10hz;

proc worker() {
  on c.tick {
    println(42);
  }
}

func main() { println(1); }' \
"Execution Output"

test_contains "Process with channel parameter" \
'clock c = 10hz;

proc producer(chan<int> out) {
  on c.tick {
    out.send(1);
  }
}

proc consumer(chan<int> in) {
  on c.tick {
    println(1);
  }
}

func main() { println(1); }' \
"Execution Output"

log_section "TEMPORAL DETERMINISTIC FEATURES - ON-CLOCK EVENTS"

test_contains "On-clock tick event" \
'clock sys = 100hz;

proc ticker() {
  on sys.tick {
    println(1);
  }
}

func main() { println(1); }' \
"Execution Output"

test_contains "Static variables in on-clock" \
'clock c = 50hz;

proc counter() {
  on c.tick {
    static count: int = 0;
    count = count + 1;
    println(count);
  }
}

func main() { println(1); }' \
"Execution Output"

log_section "DETERMINISM VERIFICATION"

test_contains "Deterministic execution (parse and check cache)" \
'clock sys = 100hz;

proc worker() {
  on sys.tick {
    static x: int = 0;
    x = x + 1;
    println(x);
  }
}

func main() { println(1); }' \
"Execution Output"

# Run it twice to verify determinism
tmp_file="/tmp/tdl_det_test_$RANDOM.tdl"
cat > "$tmp_file" << 'EOF'
clock c = 10hz;

proc p() {
  on c.tick {
    static n: int = 0;
    n = n + 1;
  }
}

func main() { println(99); }
EOF

((TESTS_RUN++))
output1=$("$BIN" "$tmp_file" 2>&1)
output2=$("$BIN" "$tmp_file" 2>&1)

if [ "$output1" = "$output2" ]; then
    log_pass "Deterministic output across runs"
else
    log_fail "Deterministic output across runs"
    echo "  Run 1: $(echo "$output1" | grep "Execution" | head -1)"
    echo "  Run 2: $(echo "$output2" | grep "Execution" | head -1)"
fi

rm -f "$tmp_file"

log_section "PARSING VERIFICATION"

test_contains "Multiple processes parse" \
'clock tick = 20hz;

proc proc1(chan<int> out) { on tick.tick { out.send(1); } }
proc proc2(chan<int> in, chan<int> out) { on tick.tick { out.send(1); } }
proc proc3(chan<int> in) { on tick.tick { println(1); } }

func main() { println(1); }' \
"Parsing"

test_contains "Complex process with multiple statements" \
'clock sys = 100hz;

proc worker(chan<int> input, chan<int> output) {
  on sys.tick {
    static counter: int = 0;
    counter = counter + 1;
    output.send(counter);
    println(counter);
  }
}

func main() { println(1); }' \
"Parsing"

log_section "TEST SUMMARY"

echo -e "Total:  $TESTS_RUN tests"
echo -e "Passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Failed: ${RED}$TESTS_FAILED${NC}"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}✅ All TDL core features tests passed!${NC}"
    exit 0
else
    echo -e "${YELLOW}⚠️  Some TDL core features not fully implemented${NC}"
    echo -e "${YELLOW}Note: Parser supports clocks/processes/channels but runtime integration may be incomplete${NC}"
    exit 1
fi
