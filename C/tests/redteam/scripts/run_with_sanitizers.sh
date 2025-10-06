
#!/bin/bash
# Run red-team tests with all sanitizers

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$(dirname "$SCRIPT_DIR")"

echo "========================================="
echo "BDI Kernel Red-Team Tests with Sanitizers"
echo "========================================="
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Build all sanitizer variants
echo -e "${BLUE}Building sanitizer variants...${NC}"
cd "$TEST_DIR"
make sanitizers

echo ""
echo "========================================="
echo "Running Address Sanitizer (ASAN) Tests"
echo "========================================="
echo ""

ASAN_FAILED=0
if make test-asan; then
    echo -e "${GREEN}ASAN tests passed${NC}"
else
    echo -e "${RED}ASAN tests failed${NC}"
    ASAN_FAILED=1
fi

echo ""
echo "========================================="
echo "Running Undefined Behavior Sanitizer (UBSAN) Tests"
echo "========================================="
echo ""

UBSAN_FAILED=0
if make test-ubsan; then
    echo -e "${GREEN}UBSAN tests passed${NC}"
else
    echo -e "${RED}UBSAN tests failed${NC}"
    UBSAN_FAILED=1
fi

echo ""
echo "========================================="
echo "Running Thread Sanitizer (TSAN) Tests"
echo "========================================="
echo ""

TSAN_FAILED=0
if make test-tsan; then
    echo -e "${GREEN}TSAN tests passed${NC}"
else
    echo -e "${RED}TSAN tests failed${NC}"
    TSAN_FAILED=1
fi

echo ""
echo "========================================="
echo "Sanitizer Test Summary"
echo "========================================="

TOTAL_FAILED=$((ASAN_FAILED + UBSAN_FAILED + TSAN_FAILED))

if [ $ASAN_FAILED -eq 0 ]; then
    echo -e "ASAN:  ${GREEN}PASSED${NC}"
else
    echo -e "ASAN:  ${RED}FAILED${NC}"
fi

if [ $UBSAN_FAILED -eq 0 ]; then
    echo -e "UBSAN: ${GREEN}PASSED${NC}"
else
    echo -e "UBSAN: ${RED}FAILED${NC}"
fi

if [ $TSAN_FAILED -eq 0 ]; then
    echo -e "TSAN:  ${GREEN}PASSED${NC}"
else
    echo -e "TSAN:  ${RED}FAILED${NC}"
fi

echo "========================================="

if [ $TOTAL_FAILED -eq 0 ]; then
    echo -e "${GREEN}All sanitizer tests passed!${NC}"
    exit 0
else
    echo -e "${RED}$TOTAL_FAILED sanitizer test suite(s) failed!${NC}"
    exit 1
fi
