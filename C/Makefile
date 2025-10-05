# Makefile for BDI Phase 2

CC = gcc
CFLAGS = -std=gnu2x -Wall -Wextra -Wpedantic -O2 -march=native
INCLUDES = -I.
LDFLAGS = -lm

# Detect AVX2 support
AVX2_TEST := $(shell echo "int main(){return 0;}" | $(CC) -mavx2 -x c - -o /dev/null 2>/dev/null && echo yes)
ifeq ($(AVX2_TEST),yes)
    CFLAGS += -mavx2 -DHAVE_AVX2
endif

# Source files
BCI_SRCS = bci/bci_arithmetic.c bci/bci_bitops.c bci/bci_conversion.c bci/bci_simd.c bci/chimera_bci.c
BTL_SRCS = btl/btl_isa.c btl/btl_regalloc.c btl/btl_scheduler.c btl/btl_peephole.c btl/chimera_btl.c

# Phase 3 source files
TYPES_EXT_SRCS = compiler/types/bci_types.c compiler/types/bci_types_extended.c
PARSER_EXT_SRCS = compiler/parser/bci_parser_extended.c compiler/ast/bci_ast.c compiler/ast/bci_ast_extended.c
SEMANTIC_EXT_SRCS = compiler/semantic_analyzer/bci_type_inference.c compiler/semantic_analyzer/bci_lifetime.c \
	            compiler/semantic_analyzer/bci_escape.c compiler/semantic_analyzer/bci_cfg.c

# Object files
BCI_OBJS = $(BCI_SRCS:.c=.o)
BTL_OBJS = $(BTL_SRCS:.c=.o)
TYPES_EXT_OBJS = $(TYPES_EXT_SRCS:.c=.o)
PARSER_EXT_OBJS = $(PARSER_EXT_SRCS:.c=.o)
SEMANTIC_EXT_OBJS = $(SEMANTIC_EXT_SRCS:.c=.o)

# Libraries
LIBBCI = libbci.a
LIBBTL = libbtl.a
LIBTYPES_EXT = libbci_types_extended.a
LIBPARSER_EXT = libbci_parser_extended.a
LIBSEMANTIC_EXT = libbci_semantic_extended.a

# Test executables
TESTS = tests/test_bci_arithmetic tests/test_bci_bitops tests/test_bci_conversion tests/test_bci_simd \
	tests/test_btl_isa tests/test_btl_regalloc tests/test_btl_scheduler tests/test_btl_peephole

# Phase 3 test executables
PHASE3_TESTS = tests/test_phase3_types tests/test_phase3_parser tests/test_phase3_semantic

# Benchmarks
BENCHMARKS = tests/benchmark_bci tests/benchmark_btl

.PHONY: all clean test benchmarks phase3 test_phase3

all: $(LIBBCI) $(LIBBTL) $(TESTS) $(BENCHMARKS)

phase3: $(LIBTYPES_EXT) $(LIBPARSER_EXT) $(LIBSEMANTIC_EXT) $(PHASE3_TESTS)

# Build libraries
$(LIBBCI): $(BCI_OBJS)
	ar rcs $@ $^

$(LIBBTL): $(BTL_OBJS)
	ar rcs $@ $^

# Build object files
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Build tests
tests/test_bci_arithmetic: tests/test_bci_arithmetic.c $(LIBBCI)
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -lbci $(LDFLAGS) -o $@

tests/test_bci_bitops: tests/test_bci_bitops.c $(LIBBCI)
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -lbci $(LDFLAGS) -o $@

tests/test_bci_conversion: tests/test_bci_conversion.c $(LIBBCI)
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -lbci $(LDFLAGS) -o $@

tests/test_bci_simd: tests/test_bci_simd.c $(LIBBCI)
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -lbci $(LDFLAGS) -o $@

tests/test_btl_isa: tests/test_btl_isa.c $(LIBBTL)
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -lbtl $(LDFLAGS) -o $@

tests/test_btl_regalloc: tests/test_btl_regalloc.c $(LIBBTL)
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -lbtl $(LDFLAGS) -o $@

tests/test_btl_scheduler: tests/test_btl_scheduler.c $(LIBBTL)
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -lbtl $(LDFLAGS) -o $@

tests/test_btl_peephole: tests/test_btl_peephole.c $(LIBBTL)
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -lbtl $(LDFLAGS) -o $@

# Build benchmarks
tests/benchmark_bci: tests/benchmark_bci.c $(LIBBCI)
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -lbci $(LDFLAGS) -o $@

tests/benchmark_btl: tests/benchmark_btl.c $(LIBBTL)
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -lbtl $(LDFLAGS) -o $@

# Run tests
test: $(TESTS)
	@echo "Running BCI tests..."
	@./tests/test_bci_arithmetic
	@./tests/test_bci_bitops
	@./tests/test_bci_conversion
	@./tests/test_bci_simd
	@echo ""
	@echo "Running BTL tests..."
	@./tests/test_btl_isa
	@./tests/test_btl_regalloc
	@./tests/test_btl_scheduler
	@./tests/test_btl_peephole

# Run benchmarks
benchmarks: $(BENCHMARKS)
	@echo "Running benchmarks..."
	@./tests/benchmark_bci
	@./tests/benchmark_btl

# Phase 3 libraries
$(LIBTYPES_EXT): $(TYPES_EXT_OBJS)
	ar rcs $@ $^

$(LIBPARSER_EXT): $(PARSER_EXT_OBJS)
	ar rcs $@ $^

$(LIBSEMANTIC_EXT): $(SEMANTIC_EXT_OBJS)
	ar rcs $@ $^

# Phase 3 tests
tests/test_phase3_types: tests/test_phase3_types.c $(LIBTYPES_EXT) $(LIBBCI)
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -lbci_types_extended -lbci $(LDFLAGS) -o $@

tests/test_phase3_parser: tests/test_phase3_parser.c $(LIBPARSER_EXT) $(LIBTYPES_EXT) $(LIBBCI)
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -lbci_parser_extended -lbci_types_extended -lbci $(LDFLAGS) -o $@

tests/test_phase3_semantic: tests/test_phase3_semantic.c $(LIBSEMANTIC_EXT) $(LIBPARSER_EXT) $(LIBTYPES_EXT) $(LIBBCI)
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -lbci_semantic_extended -lbci_parser_extended -lbci_types_extended -lbci $(LDFLAGS) -o $@

# Run Phase 3 tests
test_phase3: $(PHASE3_TESTS)
	@echo "Running Phase 3 Type System tests..."
	@./tests/test_phase3_types
	@echo ""
	@echo "Running Phase 3 Parser tests..."
	@./tests/test_phase3_parser
	@echo ""
	@echo "Running Phase 3 Semantic Analysis tests..."
	@./tests/test_phase3_semantic

clean:
	rm -f $(BCI_OBJS) $(BTL_OBJS) $(TYPES_EXT_OBJS) $(PARSER_EXT_OBJS) $(SEMANTIC_EXT_OBJS)
	rm -f $(LIBBCI) $(LIBBTL) $(LIBTYPES_EXT) $(LIBPARSER_EXT) $(LIBSEMANTIC_EXT)
	rm -f $(TESTS) $(PHASE3_TESTS) $(BENCHMARKS)

# Phase 7: VM Enhancement with JIT & GC
PHASE7_JIT_SOURCES = vm/jit/jit_compiler.c vm/jit/bytecode_compiler.c \
	             vm/jit/hot_path.c vm/jit/tiered_compilation.c
PHASE7_GC_SOURCES = vm/gc/mark_sweep.c vm/gc/generational_gc.c
PHASE7_SOURCES = $(PHASE7_JIT_SOURCES) $(PHASE7_GC_SOURCES)
PHASE7_OBJECTS = $(PHASE7_SOURCES:.c=.o)

PHASE7_TEST_SOURCES = tests/phase7/test_jit_compiler.c \
	              tests/phase7/test_mark_sweep.c \
	              tests/phase7/test_generational_gc.c \
	              tests/phase7/test_all_vm.c
PHASE7_TEST_OBJECTS = $(PHASE7_TEST_SOURCES:.c=.o)
PHASE7_TEST_BINS = tests/phase7/test_jit_compiler \
	           tests/phase7/test_mark_sweep \
	           tests/phase7/test_generational_gc \
	           tests/phase7/test_all_vm

# Phase 8.2: Graph Execution Foundation
PHASE8_GRAPH_SOURCES = vm/graph/graph.c vm/graph/graph_builder.c \
	               vm/graph/graph_executor.c vm/graph/graph_optimizer.c \
	               vm/vm_graph_integration.c
PHASE8_GRAPH_OBJECTS = $(PHASE8_GRAPH_SOURCES:.c=.o)

PHASE8_GRAPH_TEST_SOURCES = tests/phase8/test_graph_basic.c \
	                    tests/phase8/test_graph_builder.c \
	                    tests/phase8/test_graph_execution.c \
	                    tests/phase8/test_graph_optimization.c \
	                    tests/phase8/test_graph_integration.c \
	                    tests/phase8/test_graph_stress.c
PHASE8_GRAPH_TEST_OBJECTS = $(PHASE8_GRAPH_TEST_SOURCES:.c=.o)
PHASE8_GRAPH_TEST_BINS = tests/phase8/test_graph_basic \
	                 tests/phase8/test_graph_builder \
	                 tests/phase8/test_graph_execution \
	                 tests/phase8/test_graph_optimization \
	                 tests/phase8/test_graph_integration \
	                 tests/phase8/test_graph_stress

# Combined VM library with Phase 7 + Phase 8.2
VM_SOURCES = $(PHASE7_SOURCES) $(PHASE8_GRAPH_SOURCES) vm/vm.c vm/bci_vm.c vm/bci_chunk.c vm/vm_jit_integration.c
VM_OBJECTS = $(VM_SOURCES:.c=.o)

# VM library with Phase 7 and Phase 8.2 enhancements
vm_lib: $(VM_OBJECTS)
	@echo "Building VM library with JIT, GC, and Graph Execution support..."
	ar rcs libvm.a $(VM_OBJECTS)
	@echo "VM library built: libvm.a"

# Phase 7 tests
test_phase7: $(PHASE7_TEST_BINS)
	@echo "Running Phase 7 tests..."
	@./tests/phase7/test_jit_compiler
	@./tests/phase7/test_mark_sweep
	@./tests/phase7/test_generational_gc
	@./tests/phase7/test_all_vm

tests/phase7/test_jit_compiler: tests/phase7/test_jit_compiler.o $(PHASE7_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

tests/phase7/test_mark_sweep: tests/phase7/test_mark_sweep.o $(PHASE7_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

tests/phase7/test_generational_gc: tests/phase7/test_generational_gc.o $(PHASE7_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

tests/phase7/test_all_vm: tests/phase7/test_all_vm.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Phase 8.2: Graph Execution Foundation tests
test_phase8_graph: $(PHASE8_GRAPH_TEST_BINS)
	@echo "Running Phase 8.2 Graph Execution Foundation tests..."
	@./tests/phase8/test_graph_basic
	@./tests/phase8/test_graph_builder
	@./tests/phase8/test_graph_execution
	@./tests/phase8/test_graph_optimization
	@./tests/phase8/test_graph_integration
	@./tests/phase8/test_graph_stress

tests/phase8/test_graph_basic: tests/phase8/test_graph_basic.o $(VM_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $< $(VM_OBJECTS) $(LDFLAGS) -lpthread

tests/phase8/test_graph_builder: tests/phase8/test_graph_builder.o $(VM_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $< $(VM_OBJECTS) $(LDFLAGS) -lpthread

tests/phase8/test_graph_execution: tests/phase8/test_graph_execution.o $(VM_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $< $(VM_OBJECTS) $(LDFLAGS) -lpthread

tests/phase8/test_graph_optimization: tests/phase8/test_graph_optimization.o $(VM_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $< $(VM_OBJECTS) $(LDFLAGS) -lpthread

tests/phase8/test_graph_integration: tests/phase8/test_graph_integration.o $(VM_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $< $(VM_OBJECTS) $(LDFLAGS) -lpthread

tests/phase8/test_graph_stress: tests/phase8/test_graph_stress.o $(VM_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $< $(VM_OBJECTS) $(LDFLAGS) -lpthread

# Combined test suite
test_vm_complete: test_phase7 test_phase8_graph
	@echo ""
	@echo "=== Complete VM Test Suite Results ==="
	@echo "All Phase 7 and Phase 8.2 tests completed."

# Clean Phase 7 artifacts
clean_phase7:
	rm -f $(PHASE7_OBJECTS) $(PHASE7_TEST_OBJECTS) $(PHASE7_TEST_BINS)

# Clean Phase 8.2 artifacts
clean_phase8_graph:
	rm -f $(PHASE8_GRAPH_OBJECTS) $(PHASE8_GRAPH_TEST_OBJECTS) $(PHASE8_GRAPH_TEST_BINS)

# Clean all VM artifacts
clean_vm:
	rm -f $(VM_OBJECTS) libvm.a

# Comprehensive Test Infrastructure
# =================================

# Coverage compilation flags
COVERAGE_CFLAGS = -fprofile-arcs -ftest-coverage -fPIC -O0 -g
COVERAGE_LDFLAGS = -lgcov --coverage

# Test framework
TEST_FRAMEWORK_SOURCES = tests/framework/test_framework.c tests/framework/test_runner.c
TEST_FRAMEWORK_OBJECTS = $(TEST_FRAMEWORK_SOURCES:.c=.o)
TEST_FRAMEWORK_BIN = tests/framework/test_runner

# Unit test sources (150+ tests)
UNIT_TEST_SOURCES = tests/unit/vm/test_vm_core.c \
	           tests/unit/vm/test_vm_bytecode.c \
	           tests/unit/vm/test_vm_memory.c \
	           tests/unit/vm/test_vm_gc.c \
	           tests/unit/vm/test_vm_stack.c \
	           tests/unit/jit/test_jit_compiler.c \
	           tests/unit/jit/test_jit_hotspot.c \
	           tests/unit/jit/test_jit_cache.c \
	           tests/unit/jit/test_jit_optimization.c \
	           tests/unit/graph/test_graph_basic.c \
	           tests/unit/graph/test_graph_builder.c \
	           tests/unit/graph/test_graph_executor.c \
	           tests/unit/graph/test_graph_optimizer.c \
	           tests/unit/memory/test_memory_pool.c \
	           tests/unit/memory/test_memory_gc.c \
	           tests/unit/bytecode/test_bytecode_parser.c \
	           tests/unit/bytecode/test_bytecode_validator.c \
	           tests/unit/values/test_value_system.c \
	           tests/unit/values/test_type_system.c \
	           tests/unit/errors/test_error_handling.c

# Integration test sources (85+ tests)
INTEGRATION_TEST_SOURCES = tests/integration/vm_jit/test_vm_jit_integration.c \
	                  tests/integration/graph_vm/test_graph_vm_integration.c \
	                  tests/integration/jit_graph/test_jit_graph_integration.c \
	                  tests/integration/memory_system/test_memory_integration.c \
	                  tests/integration/performance/test_performance_integration.c \
	                  tests/integration/error_handling/test_error_integration.c

# Test objects
UNIT_TEST_OBJECTS = $(UNIT_TEST_SOURCES:.c=.o)
INTEGRATION_TEST_OBJECTS = $(INTEGRATION_TEST_SOURCES:.c=.o)

# Coverage targets
.PHONY: coverage coverage-build coverage-test coverage-report coverage-clean
.PHONY: test-comprehensive test-unit test-integration test-performance test-memory
.PHONY: test-all test-quick test-ci
.PHONY: vm_lib test_phase7 test_phase8_graph test_vm_complete clean_phase7 clean_phase8_graph clean_vm

# Build with coverage instrumentation
coverage-build:
	@echo "Building with coverage instrumentation..."
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) $(COVERAGE_CFLAGS)" LDFLAGS="$(LDFLAGS) $(COVERAGE_LDFLAGS)" all vm_lib

# Build test framework
test-framework: $(TEST_FRAMEWORK_BIN)

$(TEST_FRAMEWORK_BIN): $(TEST_FRAMEWORK_OBJECTS)
	@echo "Building comprehensive test framework..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lm -lpthread

# Build unit tests
test-unit-build: $(UNIT_TEST_OBJECTS)
	@echo "Unit test objects built"

# Build integration tests  
test-integration-build: $(INTEGRATION_TEST_OBJECTS)
	@echo "Integration test objects built"

# Run comprehensive test suite
test-comprehensive: test-framework
	@echo "Running comprehensive test suite..."
	./scripts/run_tests.sh

# Run unit tests only
test-unit: test-framework
	@echo "Running unit tests..."
	./scripts/run_tests.sh --unit-only

# Run integration tests only
test-integration: test-framework
	@echo "Running integration tests..."
	./scripts/run_tests.sh --integration-only

# Run performance tests
test-performance: test-framework
	@echo "Running performance tests..."
	./scripts/run_tests.sh --performance

# Run memory leak detection
test-memory: test-framework
	@echo "Running memory leak detection..."
	./scripts/run_tests.sh --no-memory-tests

# Quick test suite (essential tests only)
test-quick: test-framework
	@echo "Running quick test suite..."
	./scripts/run_tests.sh --unit-only --sequential

# CI test suite (all tests with coverage)
test-ci: coverage-build test-framework
	@echo "Running CI test suite with coverage..."
	./scripts/run_tests.sh
	$(MAKE) coverage-report

# Generate coverage reports
coverage: coverage-build test-comprehensive coverage-report

coverage-test: test-comprehensive

coverage-report:
	@echo "Generating coverage reports..."
	./scripts/generate_coverage.sh

# Clean coverage data
coverage-clean:
	@echo "Cleaning coverage data..."
	find . -name "*.gcda" -delete 2>/dev/null || true
	find . -name "*.gcno" -delete 2>/dev/null || true
	find . -name "*.gcov" -delete 2>/dev/null || true
	rm -rf coverage_reports test_results

# All tests (comprehensive suite)
test-all: test-comprehensive

# Clean test artifacts
clean-tests:
	rm -f $(TEST_FRAMEWORK_OBJECTS) $(UNIT_TEST_OBJECTS) $(INTEGRATION_TEST_OBJECTS)
	rm -f $(TEST_FRAMEWORK_BIN)
	rm -rf test_results coverage_reports

# Enhanced clean target
clean: clean-tests coverage-clean
	rm -f $(BCI_OBJS) $(BTL_OBJS) $(TYPES_EXT_OBJS) $(PARSER_EXT_OBJS) $(SEMANTIC_EXT_OBJS)
	rm -f $(LIBBCI) $(LIBBTL) $(LIBTYPES_EXT) $(LIBPARSER_EXT) $(LIBSEMANTIC_EXT)
	rm -f $(TESTS) $(PHASE3_TESTS) $(BENCHMARKS)
	rm -f $(VM_OBJECTS) libvm.a
	rm -f $(PHASE7_OBJECTS) $(PHASE7_TEST_OBJECTS) $(PHASE7_TEST_BINS)
	rm -f $(PHASE8_GRAPH_OBJECTS) $(PHASE8_GRAPH_TEST_OBJECTS) $(PHASE8_GRAPH_TEST_BINS)

# ============================================================================
# Stress and Regression Test Targets
# ============================================================================

# Stress test sources
STRESS_TEST_SOURCES = \
	tests/stress/test_stress_memory.c \
	tests/stress/test_stress_stack.c \
	tests/stress/test_stress_cpu.c \
	tests/stress/test_stress_concurrency.c \

# Regression test sources
REGRESSION_TEST_SOURCES = \
	tests/regression/test_regression_vm.c \
	tests/regression/test_regression_jit.c \
	tests/regression/test_regression_graph.c \
	tests/regression/test_regression_integration.c \

# Test utilities
TEST_UTILS_SOURCES = tests/test_utils.c
TEST_UTILS_OBJECTS = $(TEST_UTILS_SOURCES:.c=.o)

# Stress test objects and binaries
STRESS_TEST_OBJECTS = $(STRESS_TEST_SOURCES:.c=.o)
STRESS_TEST_BINS = \
	tests/stress/test_stress_memory \
	tests/stress/test_stress_stack \
	tests/stress/test_stress_cpu \
	tests/stress/test_stress_concurrency

# Regression test objects and binaries
REGRESSION_TEST_OBJECTS = $(REGRESSION_TEST_SOURCES:.c=.o)
REGRESSION_TEST_BINS = \
	tests/regression/test_regression_vm \
	tests/regression/test_regression_jit \
	tests/regression/test_regression_graph \
	tests/regression/test_regression_integration \

# Test framework dependencies
TEST_FRAMEWORK_DEPS = tests/framework/test_framework.o tests/framework/test_runner.o

# Common test libraries
TEST_LIBS = -lm -lpthread

# ============================================================================
# Build Targets
# ============================================================================

.PHONY: build-stress-tests build-regression-tests
.PHONY: stress-test stress-memory stress-stack stress-cpu stress-concurrency
.PHONY: regression-test regression-vm regression-jit regression-graph regression-integration
.PHONY: test-all-new ci-test clean-stress-tests clean-regression-tests

# Build test utilities
$(TEST_UTILS_OBJECTS): $(TEST_UTILS_SOURCES)
	$(CC) $(CFLAGS) -c $< -o $@

# Build stress tests
build-stress-tests: $(TEST_UTILS_OBJECTS) $(TEST_FRAMEWORK_DEPS) $(STRESS_TEST_BINS)

tests/stress/test_stress_memory: tests/stress/test_stress_memory.o $(TEST_UTILS_OBJECTS) $(TEST_FRAMEWORK_DEPS) vm_lib
	$(CC) $(CFLAGS) -o $@ $< $(TEST_UTILS_OBJECTS) $(TEST_FRAMEWORK_DEPS) libvm.a $(TEST_LIBS)

tests/stress/test_stress_stack: tests/stress/test_stress_stack.o $(TEST_UTILS_OBJECTS) $(TEST_FRAMEWORK_DEPS) vm_lib
	$(CC) $(CFLAGS) -o $@ $< $(TEST_UTILS_OBJECTS) $(TEST_FRAMEWORK_DEPS) libvm.a $(TEST_LIBS)

tests/stress/test_stress_cpu: tests/stress/test_stress_cpu.o $(TEST_UTILS_OBJECTS) $(TEST_FRAMEWORK_DEPS) vm_lib
	$(CC) $(CFLAGS) -o $@ $< $(TEST_UTILS_OBJECTS) $(TEST_FRAMEWORK_DEPS) libvm.a $(TEST_LIBS)

tests/stress/test_stress_concurrency: tests/stress/test_stress_concurrency.o $(TEST_UTILS_OBJECTS) $(TEST_FRAMEWORK_DEPS) vm_lib
	$(CC) $(CFLAGS) -o $@ $< $(TEST_UTILS_OBJECTS) $(TEST_FRAMEWORK_DEPS) libvm.a $(TEST_LIBS)

# Build regression tests
build-regression-tests: $(TEST_UTILS_OBJECTS) $(TEST_FRAMEWORK_DEPS) $(REGRESSION_TEST_BINS)

tests/regression/test_regression_vm: tests/regression/test_regression_vm.o $(TEST_UTILS_OBJECTS) $(TEST_FRAMEWORK_DEPS) vm_lib
	$(CC) $(CFLAGS) -o $@ $< $(TEST_UTILS_OBJECTS) $(TEST_FRAMEWORK_DEPS) libvm.a $(TEST_LIBS)

tests/regression/test_regression_jit: tests/regression/test_regression_jit.o $(TEST_UTILS_OBJECTS) $(TEST_FRAMEWORK_DEPS) vm_lib
	$(CC) $(CFLAGS) -o $@ $< $(TEST_UTILS_OBJECTS) $(TEST_FRAMEWORK_DEPS) libvm.a $(TEST_LIBS)

tests/regression/test_regression_graph: tests/regression/test_regression_graph.o $(TEST_UTILS_OBJECTS) $(TEST_FRAMEWORK_DEPS) vm_lib
	$(CC) $(CFLAGS) -o $@ $< $(TEST_UTILS_OBJECTS) $(TEST_FRAMEWORK_DEPS) libvm.a $(TEST_LIBS)

tests/regression/test_regression_integration: tests/regression/test_regression_integration.o $(TEST_UTILS_OBJECTS) $(TEST_FRAMEWORK_DEPS) vm_lib
	$(CC) $(CFLAGS) -o $@ $< $(TEST_UTILS_OBJECTS) $(TEST_FRAMEWORK_DEPS) libvm.a $(TEST_LIBS)

# ============================================================================
# Run Targets
# ============================================================================

# Run all stress tests
stress-test: build-stress-tests
	@echo "Running all stress tests..."
	./tests/stress/run_stress_tests.sh all

# Run individual stress test suites
stress-memory: build-stress-tests
	@echo "Running memory stress tests..."
	./tests/stress/run_stress_tests.sh memory

stress-stack: build-stress-tests
	@echo "Running stack stress tests..."
	./tests/stress/run_stress_tests.sh stack

stress-cpu: build-stress-tests
	@echo "Running CPU stress tests..."
	./tests/stress/run_stress_tests.sh cpu

stress-concurrency: build-stress-tests
	@echo "Running concurrency stress tests..."
	./tests/stress/run_stress_tests.sh concurrency

# Run all regression tests
regression-test: build-regression-tests
	@echo "Running all regression tests..."
	./tests/regression/run_regression_tests.sh all

# Run individual regression test suites
regression-vm: build-regression-tests
	@echo "Running VM regression tests..."
	./tests/regression/run_regression_tests.sh vm

regression-jit: build-regression-tests
	@echo "Running JIT regression tests..."
	./tests/regression/run_regression_tests.sh jit

regression-graph: build-regression-tests
	@echo "Running graph regression tests..."
	./tests/regression/run_regression_tests.sh graph

regression-integration: build-regression-tests
	@echo "Running integration regression tests..."
	./tests/regression/run_regression_tests.sh integration

# Run all tests (unit, integration, stress, regression)
test-all-new: test-comprehensive stress-test regression-test
	@echo "All tests completed!"

# CI test target
ci-test: test-ci stress-test regression-test
	@echo "CI test suite completed!"

# ============================================================================
# Clean Targets
# ============================================================================

clean-stress-tests:
	rm -f $(STRESS_TEST_OBJECTS) $(STRESS_TEST_BINS)
	rm -f tests/stress/*.o

clean-regression-tests:
	rm -f $(REGRESSION_TEST_OBJECTS) $(REGRESSION_TEST_BINS)
	rm -f tests/regression/*.o

clean-test-utils:
	rm -f $(TEST_UTILS_OBJECTS)

# Enhanced clean to include new tests
clean: clean-tests clean-stress-tests clean-regression-tests clean-test-utils coverage-clean
	rm -f $(BCI_OBJS) $(BTL_OBJS) $(TYPES_EXT_OBJS) $(PARSER_EXT_OBJS) $(SEMANTIC_EXT_OBJS)
	rm -f $(LIBBCI) $(LIBBTL) $(LIBTYPES_EXT) $(LIBPARSER_EXT) $(LIBSEMANTIC_EXT)
	rm -f $(TESTS) $(PHASE3_TESTS) $(BENCHMARKS)
	rm -f $(VM_OBJECTS) libvm.a
	rm -f $(PHASE7_OBJECTS) $(PHASE7_TEST_OBJECTS) $(PHASE7_TEST_BINS)
	rm -f $(PHASE8_GRAPH_OBJECTS) $(PHASE8_GRAPH_TEST_OBJECTS) $(PHASE8_GRAPH_TEST_BINS)


# ============================================================================
# Updated Run Targets (using shell runners)
# ============================================================================

# Override previous targets to use shell runners
stress-test: build-stress-tests
	@echo "Running all stress tests..."
	./tests/stress/run_stress_tests.sh all

stress-memory: build-stress-tests
	@echo "Running memory stress tests..."
	./tests/stress/run_stress_tests.sh memory

stress-stack: build-stress-tests
	@echo "Running stack stress tests..."
	./tests/stress/run_stress_tests.sh stack

stress-cpu: build-stress-tests
	@echo "Running CPU stress tests..."
	./tests/stress/run_stress_tests.sh cpu

stress-concurrency: build-stress-tests
	@echo "Running concurrency stress tests..."
	./tests/stress/run_stress_tests.sh concurrency

regression-test: build-regression-tests
	@echo "Running all regression tests..."
	./tests/regression/run_regression_tests.sh all

regression-vm: build-regression-tests
	@echo "Running VM regression tests..."
	./tests/regression/run_regression_tests.sh vm

regression-jit: build-regression-tests
	@echo "Running JIT regression tests..."
	./tests/regression/run_regression_tests.sh jit

regression-graph: build-regression-tests
	@echo "Running graph regression tests..."
	./tests/regression/run_regression_tests.sh graph

regression-integration: build-regression-tests
	@echo "Running integration regression tests..."
	./tests/regression/run_regression_tests.sh integration

