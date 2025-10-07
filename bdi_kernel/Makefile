
# BDI Kernel Makefile
# Build system for the Binary Decomposition Interface kernel

# ============================================================================
# Compiler and Flags
# ============================================================================

CC := gcc
CFLAGS := -Wall -Wextra -std=gnu11 -O2 -g -Wno-unused-parameter -Wno-unused-function -Wno-type-limits -D_GNU_SOURCE
LDFLAGS := -lpthread

# ============================================================================
# Directory Structure
# ============================================================================

# Test directories
TEST_DIR := tests
STRESS_TEST_DIR := $(TEST_DIR)/stress
REGRESSION_TEST_DIR := $(TEST_DIR)/regression
PROFILING_DIR := profiling

# Source files
TEST_RUNNER_SRC := $(TEST_DIR)/test_runner.c
STRESS_TEST_SRCS := $(wildcard $(STRESS_TEST_DIR)/*.c)
REGRESSION_TEST_SRCS := $(wildcard $(REGRESSION_TEST_DIR)/*.c)
PROFILING_SRCS := $(wildcard $(PROFILING_DIR)/*.c)

# Test binaries
TEST_RUNNER := test_runner
STRESS_TESTS := $(patsubst $(STRESS_TEST_DIR)/%.c,$(STRESS_TEST_DIR)/%,$(STRESS_TEST_SRCS))
REGRESSION_TESTS := $(patsubst $(REGRESSION_TEST_DIR)/%.c,$(REGRESSION_TEST_DIR)/%,$(REGRESSION_TEST_SRCS))

# Test flags
TEST_CFLAGS := $(CFLAGS) -I$(TEST_DIR) -I$(PROFILING_DIR)
TEST_LDFLAGS := $(LDFLAGS) -lpthread

# ============================================================================
# Test Targets
# ============================================================================

.PHONY: test test-unit test-integration test-stress test-regression test-performance
.PHONY: test-all test-clean build-tests

# Build all tests
build-tests: $(TEST_RUNNER) $(STRESS_TESTS) $(REGRESSION_TESTS)
	@echo "==> All tests built successfully"

# Build test runner with stress and regression test object files linked
$(TEST_RUNNER): $(TEST_RUNNER_SRC) $(STRESS_TEST_SRCS) $(REGRESSION_TEST_SRCS) $(PROFILING_SRCS)
	@echo "==> Building test runner with integrated test suites..."
	$(CC) $(TEST_CFLAGS) -DTEST_RUNNER_BUILD $(TEST_LDFLAGS) -o $@ $^

# Build individual stress test binaries (for standalone execution)
$(STRESS_TEST_DIR)/%: $(STRESS_TEST_DIR)/%.c
	@echo "==> Building stress test: $@"
	$(CC) $(TEST_CFLAGS) $(TEST_LDFLAGS) -o $@ $<

# Build individual regression test binaries (for standalone execution)
$(REGRESSION_TEST_DIR)/%: $(REGRESSION_TEST_DIR)/%.c
	@echo "==> Building regression test: $@"
	$(CC) $(TEST_CFLAGS) $(TEST_LDFLAGS) -o $@ $<

# Run all tests
test: build-tests
	@echo "==> Running all tests..."
	./$(TEST_RUNNER) --category=all --verbose

# Run unit tests
test-unit: build-tests
	@echo "==> Running unit tests..."
	./$(TEST_RUNNER) --category=unit --verbose

# Run integration tests
test-integration: build-tests
	@echo "==> Running integration tests..."
	./$(TEST_RUNNER) --category=integration --verbose

# Run stress tests
test-stress: build-tests
	@echo "==> Running stress tests..."
	./$(TEST_RUNNER) --category=stress --duration=60

# Run regression tests
test-regression: build-tests
	@echo "==> Running regression tests..."
	./$(TEST_RUNNER) --category=regression --verbose

# Run performance tests
test-performance: build-tests
	@echo "==> Running performance tests..."
	./$(TEST_RUNNER) --category=performance --output=json

# Run all test categories
test-all: test-unit test-integration test-regression test-performance
	@echo "==> All test categories completed"

# Clean test artifacts
test-clean:
	@echo "==> Cleaning test artifacts..."
	@rm -f $(TEST_RUNNER) $(STRESS_TESTS) $(REGRESSION_TESTS)
	@rm -rf test_results/
	@rm -f *.json *.csv
	@find $(TEST_DIR) -name "*.o" -delete

# ============================================================================
# Individual Stress Test Targets
# ============================================================================

.PHONY: stress-memory stress-scheduler stress-process

stress-memory: $(STRESS_TEST_DIR)/memory_stress
	@echo "==> Running memory stress test..."
	./$(STRESS_TEST_DIR)/memory_stress 60

stress-scheduler: $(STRESS_TEST_DIR)/scheduler_stress
	@echo "==> Running scheduler stress test..."
	./$(STRESS_TEST_DIR)/scheduler_stress 60

stress-process: $(STRESS_TEST_DIR)/process_stress
	@echo "==> Running process stress test..."
	./$(STRESS_TEST_DIR)/process_stress 60

# ============================================================================
# Coverage Targets
# ============================================================================

.PHONY: coverage coverage-report coverage-clean

coverage: CFLAGS += --coverage -fprofile-arcs -ftest-coverage
coverage: LDFLAGS += --coverage
coverage: test-clean build-tests
	@echo "==> Running tests with coverage..."
	./$(TEST_RUNNER) --category=all
	@echo "==> Generating coverage report..."
	@mkdir -p coverage
	gcov $(TEST_RUNNER_SRC) $(STRESS_TEST_SRCS) $(REGRESSION_TEST_SRCS)
	@mv *.gcov coverage/ 2>/dev/null || true

coverage-report: coverage
	@echo "==> Coverage Summary:"
	@grep -A 3 "File" coverage/*.gcov | head -20

coverage-clean:
	@echo "==> Cleaning coverage artifacts..."
	@rm -rf coverage/
	@find . -name "*.gcda" -o -name "*.gcno" -delete

# ============================================================================
# CI/CD Targets
# ============================================================================

.PHONY: ci-build ci-test ci-sanitizers ci-coverage

ci-build: build-tests
	@echo "==> CI: Build completed successfully"

ci-test: test-all
	@echo "==> CI: All tests completed"

ci-sanitizers: CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
ci-sanitizers: LDFLAGS += -fsanitize=address,undefined
ci-sanitizers: test-clean build-tests
	@echo "==> Running tests with sanitizers..."
	./$(TEST_RUNNER) --category=all

ci-coverage: coverage
	@echo "==> CI: Coverage report generated"

# ============================================================================
# Help Target
# ============================================================================

.PHONY: help

help:
	@echo "BDI Kernel Build System"
	@echo "======================="
	@echo ""
	@echo "Test Targets:"
	@echo "  make test                   - Run all tests"
	@echo "  make test-unit              - Run unit tests"
	@echo "  make test-integration       - Run integration tests"
	@echo "  make test-stress            - Run stress tests"
	@echo "  make test-regression        - Run regression tests"
	@echo "  make test-performance       - Run performance tests"
	@echo ""
	@echo "Individual Stress Tests:"
	@echo "  make stress-memory          - Run memory stress test"
	@echo "  make stress-scheduler       - Run scheduler stress test"
	@echo "  make stress-process         - Run process stress test"
	@echo ""
	@echo "Coverage:"
	@echo "  make coverage               - Generate coverage report"
	@echo "  make coverage-report        - Show coverage summary"
	@echo "  make coverage-clean         - Clean coverage artifacts"
	@echo ""
	@echo "CI/CD:"
	@echo "  make ci-build               - CI build"
	@echo "  make ci-test                - CI test run"
	@echo "  make ci-sanitizers          - Run with sanitizers"
	@echo "  make ci-coverage            - CI coverage"
	@echo ""
	@echo "Cleanup:"
	@echo "  make test-clean             - Clean test artifacts"

# Default target
.DEFAULT_GOAL := help
