
#!/bin/bash

# BDI Kernel Test Execution Script
# ================================

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
TEST_RESULTS_DIR="$PROJECT_ROOT/test_results"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test execution modes
RUN_UNIT_TESTS=true
RUN_INTEGRATION_TESTS=true
RUN_PERFORMANCE_TESTS=false
RUN_MEMORY_TESTS=true
PARALLEL_EXECUTION=true
VERBOSE_OUTPUT=false

# Parse command line arguments
parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --unit-only)
                RUN_UNIT_TESTS=true
                RUN_INTEGRATION_TESTS=false
                RUN_PERFORMANCE_TESTS=false
                shift
                ;;
            --integration-only)
                RUN_UNIT_TESTS=false
                RUN_INTEGRATION_TESTS=true
                RUN_PERFORMANCE_TESTS=false
                shift
                ;;
            --performance)
                RUN_PERFORMANCE_TESTS=true
                shift
                ;;
            --no-memory-tests)
                RUN_MEMORY_TESTS=false
                shift
                ;;
            --sequential)
                PARALLEL_EXECUTION=false
                shift
                ;;
            --verbose)
                VERBOSE_OUTPUT=true
                shift
                ;;
            --help)
                show_help
                exit 0
                ;;
            *)
                echo "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
}

# Show help message
show_help() {
    cat << EOF
BDI Kernel Test Runner

Usage: $0 [OPTIONS]

Options:
    --unit-only         Run only unit tests
    --integration-only  Run only integration tests
    --performance       Include performance tests
    --no-memory-tests   Skip memory leak detection
    --sequential        Run tests sequentially (not in parallel)
    --verbose           Enable verbose output
    --help              Show this help message

Examples:
    $0                          # Run all tests (default)
    $0 --unit-only             # Run only unit tests
    $0 --performance --verbose # Run all tests including performance with verbose output
EOF
}

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Create test results directory
create_test_directories() {
    log_info "Creating test result directories..."
    mkdir -p "$TEST_RESULTS_DIR"/{unit,integration,performance,memory}
}

# Build test executables
build_tests() {
    log_info "Building test executables..."
    cd "$PROJECT_ROOT"
    
    # Build main project
    make clean
    make all vm_lib
    
    # Build test framework
    if [ -f "tests/framework/test_framework.c" ]; then
        log_info "Building comprehensive test framework..."
        gcc -std=gnu2x -Wall -Wextra -O2 -I. \
            tests/framework/test_framework.c \
            tests/framework/test_runner.c \
            -o tests/framework/test_runner \
            -lm -lpthread
    fi
    
    # Build existing tests
    if make test_phase7 test_phase8_graph >/dev/null 2>&1; then
        log_info "Building existing test suites..."
        make test_phase7 test_phase8_graph
    fi
    
    log_success "Test build completed"
}

# Run unit tests
run_unit_tests() {
    if [ "$RUN_UNIT_TESTS" != true ]; then
        return 0
    fi
    
    log_info "Running unit tests..."
    local unit_passed=0
    local unit_total=0
    
    # Run comprehensive unit test suite
    if [ -f "tests/framework/test_runner" ]; then
        log_info "Executing comprehensive unit test suite..."
        if [ "$VERBOSE_OUTPUT" = true ]; then
            ./tests/framework/test_runner | tee "$TEST_RESULTS_DIR/unit/comprehensive_tests.log"
        else
            ./tests/framework/test_runner > "$TEST_RESULTS_DIR/unit/comprehensive_tests.log" 2>&1
        fi
        
        if [ $? -eq 0 ]; then
            log_success "Comprehensive unit tests passed"
            ((unit_passed++))
        else
            log_error "Comprehensive unit tests failed"
        fi
        ((unit_total++))
    fi
    
    # Run existing unit tests
    if make test >/dev/null 2>&1; then
        log_info "Executing existing unit tests..."
        if [ "$VERBOSE_OUTPUT" = true ]; then
            make test | tee "$TEST_RESULTS_DIR/unit/existing_tests.log"
        else
            make test > "$TEST_RESULTS_DIR/unit/existing_tests.log" 2>&1
        fi
        
        if [ $? -eq 0 ]; then
            log_success "Existing unit tests passed"
            ((unit_passed++))
        else
            log_error "Existing unit tests failed"
        fi
        ((unit_total++))
    fi
    
    log_info "Unit tests completed: $unit_passed/$unit_total suites passed"
    return $((unit_total - unit_passed))
}

# Run integration tests
run_integration_tests() {
    if [ "$RUN_INTEGRATION_TESTS" != true ]; then
        return 0
    fi
    
    log_info "Running integration tests..."
    local integration_passed=0
    local integration_total=0
    
    # Run VM integration tests
    if make test_vm_complete >/dev/null 2>&1; then
        log_info "Executing VM integration tests..."
        if [ "$VERBOSE_OUTPUT" = true ]; then
            make test_vm_complete | tee "$TEST_RESULTS_DIR/integration/vm_tests.log"
        else
            make test_vm_complete > "$TEST_RESULTS_DIR/integration/vm_tests.log" 2>&1
        fi
        
        if [ $? -eq 0 ]; then
            log_success "VM integration tests passed"
            ((integration_passed++))
        else
            log_error "VM integration tests failed"
        fi
        ((integration_total++))
    fi
    
    # Run Phase 7 tests
    if make test_phase7 >/dev/null 2>&1; then
        log_info "Executing Phase 7 integration tests..."
        if [ "$VERBOSE_OUTPUT" = true ]; then
            make test_phase7 | tee "$TEST_RESULTS_DIR/integration/phase7_tests.log"
        else
            make test_phase7 > "$TEST_RESULTS_DIR/integration/phase7_tests.log" 2>&1
        fi
        
        if [ $? -eq 0 ]; then
            log_success "Phase 7 integration tests passed"
            ((integration_passed++))
        else
            log_error "Phase 7 integration tests failed"
        fi
        ((integration_total++))
    fi
    
    # Run Phase 8 Graph tests
    if make test_phase8_graph >/dev/null 2>&1; then
        log_info "Executing Phase 8 Graph integration tests..."
        if [ "$VERBOSE_OUTPUT" = true ]; then
            make test_phase8_graph | tee "$TEST_RESULTS_DIR/integration/phase8_graph_tests.log"
        else
            make test_phase8_graph > "$TEST_RESULTS_DIR/integration/phase8_graph_tests.log" 2>&1
        fi
        
        if [ $? -eq 0 ]; then
            log_success "Phase 8 Graph integration tests passed"
            ((integration_passed++))
        else
            log_error "Phase 8 Graph integration tests failed"
        fi
        ((integration_total++))
    fi
    
    log_info "Integration tests completed: $integration_passed/$integration_total suites passed"
    return $((integration_total - integration_passed))
}

# Run performance tests
run_performance_tests() {
    if [ "$RUN_PERFORMANCE_TESTS" != true ]; then
        return 0
    fi
    
    log_info "Running performance tests..."
    local performance_passed=0
    local performance_total=0
    
    # Run benchmarks if available
    if make benchmarks >/dev/null 2>&1; then
        log_info "Executing performance benchmarks..."
        if [ "$VERBOSE_OUTPUT" = true ]; then
            make benchmarks | tee "$TEST_RESULTS_DIR/performance/benchmarks.log"
        else
            make benchmarks > "$TEST_RESULTS_DIR/performance/benchmarks.log" 2>&1
        fi
        
        if [ $? -eq 0 ]; then
            log_success "Performance benchmarks completed"
            ((performance_passed++))
        else
            log_error "Performance benchmarks failed"
        fi
        ((performance_total++))
    fi
    
    log_info "Performance tests completed: $performance_passed/$performance_total suites passed"
    return $((performance_total - performance_passed))
}

# Run memory leak detection
run_memory_tests() {
    if [ "$RUN_MEMORY_TESTS" != true ]; then
        return 0
    fi
    
    log_info "Running memory leak detection..."
    
    if ! command -v valgrind >/dev/null 2>&1; then
        log_warning "Valgrind not found, skipping memory leak detection"
        return 0
    fi
    
    local memory_issues=0
    
    # Test comprehensive test suite with valgrind
    if [ -f "tests/framework/test_runner" ]; then
        log_info "Running memory leak detection on comprehensive tests..."
        valgrind --leak-check=full \
                 --show-leak-kinds=all \
                 --track-origins=yes \
                 --verbose \
                 --log-file="$TEST_RESULTS_DIR/memory/comprehensive_valgrind.log" \
                 ./tests/framework/test_runner >/dev/null 2>&1
        
        # Check for memory leaks
        if grep -q "ERROR SUMMARY: 0 errors" "$TEST_RESULTS_DIR/memory/comprehensive_valgrind.log"; then
            log_success "No memory leaks detected in comprehensive tests"
        else
            log_error "Memory leaks detected in comprehensive tests"
            ((memory_issues++))
        fi
    fi
    
    # Test existing executables with valgrind
    for test_exe in tests/test_*; do
        if [ -x "$test_exe" ]; then
            test_name=$(basename "$test_exe")
            log_info "Running memory leak detection on $test_name..."
            
            valgrind --leak-check=full \
                     --show-leak-kinds=all \
                     --track-origins=yes \
                     --log-file="$TEST_RESULTS_DIR/memory/${test_name}_valgrind.log" \
                     "$test_exe" >/dev/null 2>&1
            
            if grep -q "ERROR SUMMARY: 0 errors" "$TEST_RESULTS_DIR/memory/${test_name}_valgrind.log"; then
                log_success "No memory leaks detected in $test_name"
            else
                log_error "Memory leaks detected in $test_name"
                ((memory_issues++))
            fi
        fi
    done
    
    if [ $memory_issues -eq 0 ]; then
        log_success "Memory leak detection completed - no issues found"
        return 0
    else
        log_error "Memory leak detection found $memory_issues issues"
        return $memory_issues
    fi
}

# Generate test summary report
generate_test_summary() {
    log_info "Generating test summary report..."
    
    local summary_file="$TEST_RESULTS_DIR/test_summary.txt"
    
    cat > "$summary_file" << EOF
BDI Kernel Test Execution Summary
================================
Date: $(date)
Test Configuration:
- Unit Tests: $RUN_UNIT_TESTS
- Integration Tests: $RUN_INTEGRATION_TESTS
- Performance Tests: $RUN_PERFORMANCE_TESTS
- Memory Tests: $RUN_MEMORY_TESTS
- Parallel Execution: $PARALLEL_EXECUTION

Test Results:
EOF
    
    # Add unit test results
    if [ "$RUN_UNIT_TESTS" = true ] && [ -f "$TEST_RESULTS_DIR/unit/comprehensive_tests.log" ]; then
        echo "" >> "$summary_file"
        echo "Unit Tests:" >> "$summary_file"
        grep -E "(PASS|FAIL|Test Summary)" "$TEST_RESULTS_DIR/unit/comprehensive_tests.log" | tail -10 >> "$summary_file"
    fi
    
    # Add integration test results
    if [ "$RUN_INTEGRATION_TESTS" = true ]; then
        echo "" >> "$summary_file"
        echo "Integration Tests:" >> "$summary_file"
        for log_file in "$TEST_RESULTS_DIR/integration"/*.log; do
            if [ -f "$log_file" ]; then
                echo "$(basename "$log_file"):" >> "$summary_file"
                grep -E "(PASS|FAIL|completed)" "$log_file" | tail -3 >> "$summary_file"
            fi
        done
    fi
    
    # Add memory test results
    if [ "$RUN_MEMORY_TESTS" = true ]; then
        echo "" >> "$summary_file"
        echo "Memory Leak Detection:" >> "$summary_file"
        for valgrind_log in "$TEST_RESULTS_DIR/memory"/*_valgrind.log; do
            if [ -f "$valgrind_log" ]; then
                echo "$(basename "$valgrind_log"):" >> "$summary_file"
                grep "ERROR SUMMARY" "$valgrind_log" >> "$summary_file"
            fi
        done
    fi
    
    log_success "Test summary generated: $summary_file"
}

# Main execution function
main() {
    log_info "Starting BDI Kernel test execution..."
    
    parse_arguments "$@"
    create_test_directories
    build_tests
    
    local total_failures=0
    
    # Run test suites
    run_unit_tests
    total_failures=$((total_failures + $?))
    
    run_integration_tests
    total_failures=$((total_failures + $?))
    
    run_performance_tests
    total_failures=$((total_failures + $?))
    
    run_memory_tests
    total_failures=$((total_failures + $?))
    
    generate_test_summary
    
    # Final results
    if [ $total_failures -eq 0 ]; then
        log_success "All tests completed successfully!"
        log_info "Test results available in: $TEST_RESULTS_DIR"
        exit 0
    else
        log_error "Test execution completed with $total_failures failures"
        log_info "Test results available in: $TEST_RESULTS_DIR"
        exit 1
    fi
}

# Execute main function
main "$@"
