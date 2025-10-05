#!/bin/bash
# Stress Test Runner Script

SUITE="${1:-all}"

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║         BDI Kernel Stress Test Suite Runner               ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

PASSED=0
FAILED=0
START_TIME=$(date +%s)

run_test() {
    local name=$1
    local binary=$2
    
    echo "--- Running $name Stress Tests ---"
    if [ -x "$binary" ]; then
        if $binary; then
            echo "✓ $name stress tests PASSED"
            ((PASSED++))
        else
            echo "✗ $name stress tests FAILED"
            ((FAILED++))
        fi
    else
        echo "✗ $name test binary not found: $binary"
        ((FAILED++))
    fi
    echo ""
}

case "$SUITE" in
    all)
        run_test "Memory" "./tests/stress/test_stress_memory"
        run_test "Stack" "./tests/stress/test_stress_stack"
        run_test "CPU" "./tests/stress/test_stress_cpu"
        run_test "Concurrency" "./tests/stress/test_stress_concurrency"
        ;;
    memory)
        run_test "Memory" "./tests/stress/test_stress_memory"
        ;;
    stack)
        run_test "Stack" "./tests/stress/test_stress_stack"
        ;;
    cpu)
        run_test "CPU" "./tests/stress/test_stress_cpu"
        ;;
    concurrency)
        run_test "Concurrency" "./tests/stress/test_stress_concurrency"
        ;;
    *)
        echo "Unknown test suite: $SUITE"
        echo "Available: all, memory, stack, cpu, concurrency"
        exit 1
        ;;
esac

END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))

echo "╔════════════════════════════════════════════════════════════╗"
echo "║                    Test Summary                            ║"
echo "╠════════════════════════════════════════════════════════════╣"
printf "║  Suites Passed: %-3d                                       ║\n" $PASSED
printf "║  Suites Failed: %-3d                                       ║\n" $FAILED
printf "║  Total Time:    %-3d seconds                              ║\n" $ELAPSED
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

exit $FAILED
