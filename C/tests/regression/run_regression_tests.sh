#!/bin/bash
# Regression Test Runner Script

SUITE="${1:-all}"

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║       BDI Kernel Regression Test Suite Runner             ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

PASSED=0
FAILED=0
START_TIME=$(date +%s)

run_test() {
    local name=$1
    local binary=$2
    
    echo "--- Running $name Regression Tests ---"
    if [ -x "$binary" ]; then
        if $binary; then
            echo "✓ $name regression tests PASSED"
            ((PASSED++))
        else
            echo "✗ $name regression tests FAILED"
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
        run_test "VM" "./tests/regression/test_regression_vm"
        run_test "JIT" "./tests/regression/test_regression_jit"
        run_test "Graph" "./tests/regression/test_regression_graph"
        run_test "Integration" "./tests/regression/test_regression_integration"
        ;;
    vm)
        run_test "VM" "./tests/regression/test_regression_vm"
        ;;
    jit)
        run_test "JIT" "./tests/regression/test_regression_jit"
        ;;
    graph)
        run_test "Graph" "./tests/regression/test_regression_graph"
        ;;
    integration)
        run_test "Integration" "./tests/regression/test_regression_integration"
        ;;
    *)
        echo "Unknown test suite: $SUITE"
        echo "Available: all, vm, jit, graph, integration"
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
