
#!/bin/bash
# Run all red-team memory tests

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$TEST_DIR/build"

echo "========================================="
echo "BDI Kernel Red-Team Memory Test Suite"
echo "========================================="
echo ""

# Check if tests are built
if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: Tests not built. Run 'make' first."
    exit 1
fi

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test categories
CATEGORIES=(
    "allocator"
    "numa"
    "hugepage"
    "pmm"
    "vmm"
    "ham"
    "instrumentation"
)

# Track results
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Run tests for each category
for category in "${CATEGORIES[@]}"; do
    test_exe="$BUILD_DIR/$category/test_${category}_redteam"
    
    if [ ! -f "$test_exe" ]; then
        echo -e "${YELLOW}[SKIP]${NC} $category tests (not found)"
        continue
    fi
    
    echo "Running $category tests..."
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    if "$test_exe"; then
        echo -e "${GREEN}[PASS]${NC} $category tests"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}[FAIL]${NC} $category tests"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    
    echo ""
done

# Print summary
echo "========================================="
echo "Test Summary"
echo "========================================="
echo "Total:  $TOTAL_TESTS"
echo -e "Passed: ${GREEN}$PASSED_TESTS${NC}"
echo -e "Failed: ${RED}$FAILED_TESTS${NC}"
echo "========================================="

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi
