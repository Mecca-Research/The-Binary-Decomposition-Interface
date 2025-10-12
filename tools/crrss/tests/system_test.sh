#!/bin/bash
# ============================================================================
# CRRSS Comprehensive System Test Suite
# Phase 2 Stage 5: Enhanced Build System Integration
# ============================================================================

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Configuration
CRRSS_TOOL="./build/bin/crrss"
TEST_DIR="./tests"
TEMP_DIR="/tmp/crrss_system_test_$$"
REPORT_FILE="$TEMP_DIR/test_report.txt"

# Create temp directory
mkdir -p "$TEMP_DIR"
trap "rm -rf $TEMP_DIR" EXIT

# ============================================================================
# Helper Functions
# ============================================================================

print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

print_test() {
    echo -e "${CYAN}[TEST]${NC} $1"
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
}

print_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
    PASSED_TESTS=$((PASSED_TESTS + 1))
}

print_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    FAILED_TESTS=$((FAILED_TESTS + 1))
}

print_skip() {
    echo -e "${YELLOW}[SKIP]${NC} $1"
}

run_test() {
    local test_name="$1"
    local command="$2"
    local expected_rc="${3:-0}"
    
    print_test "$test_name"
    
    if eval "$command" > /dev/null 2>&1; then
        actual_rc=0
    else
        actual_rc=$?
    fi
    
    if [ "$actual_rc" -eq "$expected_rc" ]; then
        print_pass "$test_name"
        return 0
    else
        print_fail "$test_name (expected rc=$expected_rc, got rc=$actual_rc)"
        return 1
    fi
}

# ============================================================================
# Pre-Flight Checks
# ============================================================================

print_header "CRRSS System Test Suite"
echo "Test Directory: $(pwd)"
echo "Temp Directory: $TEMP_DIR"
echo ""

# Check if CRRSS tool exists
if [ ! -f "$CRRSS_TOOL" ]; then
    echo -e "${RED}Error:${NC} CRRSS tool not found at $CRRSS_TOOL"
    echo "Please run 'make tool' first"
    exit 1
fi

echo -e "${GREEN}✓${NC} CRRSS tool found: $CRRSS_TOOL"
echo ""

# ============================================================================
# Test 1: Basic CLI Help and Version
# ============================================================================

print_header "Test Suite 1: Basic CLI Functionality"

run_test "Display help message" "$CRRSS_TOOL --help"
run_test "Display version" "$CRRSS_TOOL --version"
run_test "Run without arguments (should fail)" "$CRRSS_TOOL" 1

echo ""

# ============================================================================
# Test 2: Query Command Tests
# ============================================================================

print_header "Test Suite 2: Query Command"

# Create test file for query
cat > "$TEMP_DIR/test_query.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>

void test_memory_leak() {
    int *ptr = malloc(sizeof(int) * 100);
    // Oops, forgot to free!
}

void test_null_deref() {
    int *ptr = NULL;
    *ptr = 42;  // NULL dereference
}

int main() {
    return 0;
}
EOF

run_test "Query by priority P0" "$CRRSS_TOOL query --priority P0 --max-results 10"
run_test "Query by priority P1" "$CRRSS_TOOL query --priority P1 --max-results 10"
run_test "Query by category memory" "$CRRSS_TOOL query --category memory --max-results 10"
run_test "Query with file analysis" "$CRRSS_TOOL query --file $TEMP_DIR/test_query.c --max-results 10"

# Test max-results boundary
run_test "Query with max-results=1000" "$CRRSS_TOOL query --priority P0 --max-results 1000"
run_test "Query with max-results=2000 (should clamp)" "$CRRSS_TOOL query --priority P0 --max-results 2000"

echo ""

# ============================================================================
# Test 3: MSM Command Tests
# ============================================================================

print_header "Test Suite 3: MSM Command"

# Create test file with memory issues
cat > "$TEMP_DIR/test_msm.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void buffer_overflow() {
    char buffer[10];
    strcpy(buffer, "This is way too long for buffer");
}

void use_after_free() {
    int *ptr = malloc(sizeof(int));
    free(ptr);
    *ptr = 42;  // Use after free!
}

void double_free() {
    int *ptr = malloc(sizeof(int));
    free(ptr);
    free(ptr);  // Double free!
}

int main() {
    buffer_overflow();
    use_after_free();
    double_free();
    return 0;
}
EOF

run_test "MSM analyze file" "$CRRSS_TOOL msm --file $TEMP_DIR/test_msm.c --report $TEMP_DIR/msm_report.txt"
run_test "MSM with max-issues=100" "$CRRSS_TOOL msm --file $TEMP_DIR/test_msm.c --max-issues 100"
run_test "MSM with max-issues=1000" "$CRRSS_TOOL msm --file $TEMP_DIR/test_msm.c --max-issues 1000"
run_test "MSM with max-issues=2000 (should clamp)" "$CRRSS_TOOL msm --file $TEMP_DIR/test_msm.c --max-issues 2000"

# Test report file was created
if [ -f "$TEMP_DIR/msm_report.txt" ]; then
    print_pass "MSM report file created"
else
    print_fail "MSM report file not created"
fi

echo ""

# ============================================================================
# Test 4: STP Command Tests
# ============================================================================

print_header "Test Suite 4: STP Command"

# Create test file for STP
cat > "$TEMP_DIR/test_stp.c" << 'EOF'
#include <stdio.h>
#include <unistd.h>

void inefficient_loop() {
    for (int i = 0; i < 1000000; i++) {
        usleep(1);  // Very inefficient!
    }
}

void unnecessary_allocation() {
    for (int i = 0; i < 1000; i++) {
        int *ptr = malloc(sizeof(int));
        free(ptr);  // Allocate/free in loop
    }
}

int main() {
    inefficient_loop();
    unnecessary_allocation();
    return 0;
}
EOF

run_test "STP analyze file" "$CRRSS_TOOL stp --file $TEMP_DIR/test_stp.c --report $TEMP_DIR/stp_report.txt"
run_test "STP with max-issues=100" "$CRRSS_TOOL stp --file $TEMP_DIR/test_stp.c --max-issues 100"
run_test "STP with max-issues=1000" "$CRRSS_TOOL stp --file $TEMP_DIR/test_stp.c --max-issues 1000"
run_test "STP with max-issues=2000 (should clamp)" "$CRRSS_TOOL stp --file $TEMP_DIR/test_stp.c --max-issues 2000"

# Test report file was created
if [ -f "$TEMP_DIR/stp_report.txt" ]; then
    print_pass "STP report file created"
else
    print_fail "STP report file not created"
fi

echo ""

# ============================================================================
# Test 5: Validate Command Tests
# ============================================================================

print_header "Test Suite 5: Validate Command"

# Create test directory with multiple files
mkdir -p "$TEMP_DIR/validate_test"
cp "$TEMP_DIR/test_query.c" "$TEMP_DIR/validate_test/"
cp "$TEMP_DIR/test_msm.c" "$TEMP_DIR/validate_test/"
cp "$TEMP_DIR/test_stp.c" "$TEMP_DIR/validate_test/"

run_test "Validate single file" "$CRRSS_TOOL validate --file $TEMP_DIR/test_query.c --report $TEMP_DIR/validate_report.txt"
run_test "Validate directory" "$CRRSS_TOOL validate --directory $TEMP_DIR/validate_test --report $TEMP_DIR/validate_dir_report.txt"
run_test "Validate with max-issues=100" "$CRRSS_TOOL validate --file $TEMP_DIR/test_query.c --max-issues 100"
run_test "Validate with max-issues=1000" "$CRRSS_TOOL validate --file $TEMP_DIR/test_query.c --max-issues 1000"
run_test "Validate with max-issues=2000 (should clamp)" "$CRRSS_TOOL validate --file $TEMP_DIR/test_query.c --max-issues 2000"

# Test report files were created
if [ -f "$TEMP_DIR/validate_report.txt" ]; then
    print_pass "Validate report file created"
else
    print_fail "Validate report file not created"
fi

if [ -f "$TEMP_DIR/validate_dir_report.txt" ]; then
    print_pass "Validate directory report file created"
else
    print_fail "Validate directory report file not created"
fi

echo ""

# ============================================================================
# Test 6: Buffer Overflow Security Tests (PR#182)
# ============================================================================

print_header "Test Suite 6: Buffer Overflow Security (PR#182)"

print_test "Testing buffer overflow fixes from PR#182"

# Test Query command with large max-results (should clamp to 1000)
if $CRRSS_TOOL query --priority P0 --max-results 5000 2>&1 | grep -q "clamping"; then
    print_pass "Query command correctly clamps max-results > 1000"
else
    print_fail "Query command does not clamp max-results properly"
fi

# Test MSM command with large max-issues (should clamp to 1000)
if $CRRSS_TOOL msm --file "$TEMP_DIR/test_msm.c" --max-issues 5000 2>&1 | grep -q "clamping"; then
    print_pass "MSM command correctly clamps max-issues > 1000"
else
    print_fail "MSM command does not clamp max-issues properly"
fi

# Test STP command with large max-issues (should clamp to 1000)
if $CRRSS_TOOL stp --file "$TEMP_DIR/test_stp.c" --max-issues 5000 2>&1 | grep -q "clamping"; then
    print_pass "STP command correctly clamps max-issues > 1000"
else
    print_fail "STP command does not clamp max-issues properly"
fi

# Test Validate command with large max-issues (should clamp to 1000)
if $CRRSS_TOOL validate --file "$TEMP_DIR/test_query.c" --max-issues 5000 2>&1 | grep -q "clamping"; then
    print_pass "Validate command correctly clamps max-issues > 1000"
else
    print_fail "Validate command does not clamp max-issues properly"
fi

echo ""

# ============================================================================
# Test 7: Edge Cases and Error Handling
# ============================================================================

print_header "Test Suite 7: Edge Cases and Error Handling"

# Test non-existent file
run_test "Handle non-existent file" "$CRRSS_TOOL msm --file /nonexistent/file.c" 1

# Test non-existent directory
run_test "Handle non-existent directory" "$CRRSS_TOOL validate --directory /nonexistent/dir" 1

# Test invalid command
run_test "Handle invalid command" "$CRRSS_TOOL invalid_command" 1

# Test missing required argument
run_test "Handle missing file argument" "$CRRSS_TOOL msm" 1

# Test empty file
touch "$TEMP_DIR/empty.c"
run_test "Handle empty file" "$CRRSS_TOOL msm --file $TEMP_DIR/empty.c"

# Test very large file name
LONG_NAME="$TEMP_DIR/$(printf 'a%.0s' {1..200}).c"
echo "int main() { return 0; }" > "$LONG_NAME"
run_test "Handle long filename" "$CRRSS_TOOL msm --file $LONG_NAME"

echo ""

# ============================================================================
# Test 8: Report Format Tests
# ============================================================================

print_header "Test Suite 8: Report Format Tests"

run_test "Generate text report" "$CRRSS_TOOL msm --file $TEMP_DIR/test_msm.c --report $TEMP_DIR/report.txt --format text"
run_test "Generate JSON report" "$CRRSS_TOOL msm --file $TEMP_DIR/test_msm.c --report $TEMP_DIR/report.json --format json"
run_test "Generate HTML report" "$CRRSS_TOOL msm --file $TEMP_DIR/test_msm.c --report $TEMP_DIR/report.html --format html"

# Verify report files exist
for format in txt json html; do
    if [ -f "$TEMP_DIR/report.$format" ]; then
        print_pass "Report format: $format"
    else
        print_fail "Report format: $format not created"
    fi
done

echo ""

# ============================================================================
# Test 9: CRRSS Integration Tests
# ============================================================================

print_header "Test Suite 9: CRRSS Component Integration"

# Test that all components work together
print_test "Integration: Query + MSM"
if $CRRSS_TOOL query --file "$TEMP_DIR/test_query.c" --max-results 10 > /dev/null 2>&1 && \
   $CRRSS_TOOL msm --file "$TEMP_DIR/test_query.c" --max-issues 10 > /dev/null 2>&1; then
    print_pass "Query + MSM integration works"
else
    print_fail "Query + MSM integration failed"
fi

print_test "Integration: MSM + STP"
if $CRRSS_TOOL msm --file "$TEMP_DIR/test_msm.c" --max-issues 10 > /dev/null 2>&1 && \
   $CRRSS_TOOL stp --file "$TEMP_DIR/test_msm.c" --max-issues 10 > /dev/null 2>&1; then
    print_pass "MSM + STP integration works"
else
    print_fail "MSM + STP integration failed"
fi

print_test "Integration: Full validation pipeline"
if $CRRSS_TOOL validate --directory "$TEMP_DIR/validate_test" --report "$TEMP_DIR/full_validate.txt" > /dev/null 2>&1; then
    print_pass "Full validation pipeline works"
else
    print_fail "Full validation pipeline failed"
fi

echo ""

# ============================================================================
# Test 10: Performance Tests
# ============================================================================

print_header "Test Suite 10: Performance Tests"

# Create larger test file
cat > "$TEMP_DIR/large_test.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>

// Generate multiple functions
EOF

for i in {1..50}; do
    cat >> "$TEMP_DIR/large_test.c" << EOF
void function_$i() {
    int *ptr = malloc(sizeof(int) * 100);
    free(ptr);
}
EOF
done

echo "int main() { return 0; }" >> "$TEMP_DIR/large_test.c"

print_test "Performance: Analyze large file"
START_TIME=$(date +%s)
if $CRRSS_TOOL msm --file "$TEMP_DIR/large_test.c" --max-issues 100 > /dev/null 2>&1; then
    END_TIME=$(date +%s)
    ELAPSED=$((END_TIME - START_TIME))
    if [ "$ELAPSED" -lt 30 ]; then
        print_pass "Large file analysis completed in ${ELAPSED}s (< 30s)"
    else
        print_fail "Large file analysis took too long: ${ELAPSED}s"
    fi
else
    print_fail "Large file analysis failed"
fi

echo ""

# ============================================================================
# Final Summary
# ============================================================================

print_header "Test Summary"

echo -e "Total Tests:  ${CYAN}$TOTAL_TESTS${NC}"
echo -e "Passed:       ${GREEN}$PASSED_TESTS${NC}"
echo -e "Failed:       ${RED}$FAILED_TESTS${NC}"

if [ "$FAILED_TESTS" -eq 0 ]; then
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}All tests passed! ✓${NC}"
    echo -e "${GREEN}========================================${NC}"
    exit 0
else
    echo ""
    echo -e "${RED}========================================${NC}"
    echo -e "${RED}Some tests failed! ✗${NC}"
    echo -e "${RED}========================================${NC}"
    exit 1
fi
