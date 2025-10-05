
# ===================================================================
# Makefile for Phase 8 Tests - Enhanced with JIT Integration
# ===================================================================

CC = gcc
CFLAGS = -std=c2x -Wall -Wextra -Werror -O2 -g
INCLUDES = -I../.. -I../../compiler -I../../vm
LDFLAGS = -lm -lpthread

# Source directories
COMPILER_DIR = ../../compiler
VM_DIR = ../../vm
GC_DIR = ../../vm/gc
JIT_DIR = ../../vm/jit

# Source files
LEXER_SRC = $(COMPILER_DIR)/lexer/bci_lexer.c
PARSER_SRC = $(COMPILER_DIR)/parser/bci_parser.c
AST_SRC = $(COMPILER_DIR)/ast/bci_ast.c
CODEGEN_SRC = $(COMPILER_DIR)/codegen/codegen.c
CHUNK_SRC = $(VM_DIR)/bci_chunk.c
VM_SRC = $(VM_DIR)/bci_vm.c $(VM_DIR)/vm.c
GC_SRC = $(GC_DIR)/mark_sweep.c $(GC_DIR)/generational_gc.c
JIT_SRC = $(JIT_DIR)/jit_compiler.c $(JIT_DIR)/hot_path.c $(JIT_DIR)/tiered_compilation.c $(JIT_DIR)/bytecode_compiler.c
JIT_INTEGRATION_SRC = $(VM_DIR)/vm_jit_integration.c
PIPELINE_SRC = ../../bdi_pipeline.c

# All source files
ALL_SRC = $(LEXER_SRC) $(PARSER_SRC) $(AST_SRC) $(CODEGEN_SRC) \
          $(CHUNK_SRC) $(VM_SRC) $(GC_SRC) $(JIT_SRC) $(JIT_INTEGRATION_SRC) $(PIPELINE_SRC)

# Test executables - Original Phase 8 tests
TEST_BYTECODE = test_bytecode_generation
TEST_E2E = test_e2e_pipeline

# JIT Integration test executables
TEST_JIT_BASIC = test_jit_basic
TEST_JIT_CORRECTNESS = test_jit_correctness
TEST_JIT_PERFORMANCE = test_jit_performance
TEST_JIT_INTEGRATION = test_jit_integration
TEST_JIT_STRESS = test_jit_stress
TEST_JIT_BENCHMARKS = test_jit_benchmarks

# All JIT tests
JIT_TESTS = $(TEST_JIT_BASIC) $(TEST_JIT_CORRECTNESS) $(TEST_JIT_PERFORMANCE) \
            $(TEST_JIT_INTEGRATION) $(TEST_JIT_STRESS) $(TEST_JIT_BENCHMARKS)

# All tests
ALL_TESTS = $(TEST_BYTECODE) $(TEST_E2E) $(JIT_TESTS)

.PHONY: all clean test test-original test-jit test-all help \
        test-bytecode test-e2e test-jit-basic test-jit-correctness \
        test-jit-performance test-jit-integration test-jit-stress test-jit-benchmarks

# Default target - build all tests
all: $(ALL_TESTS)

# Original Phase 8 tests
$(TEST_BYTECODE): test_bytecode_generation.c $(ALL_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

$(TEST_E2E): test_e2e_pipeline.c $(ALL_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

# JIT Integration tests
$(TEST_JIT_BASIC): test_jit_basic.c $(ALL_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

$(TEST_JIT_CORRECTNESS): test_jit_correctness.c $(ALL_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

$(TEST_JIT_PERFORMANCE): test_jit_performance.c $(ALL_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

$(TEST_JIT_INTEGRATION): test_jit_integration.c $(ALL_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

$(TEST_JIT_STRESS): test_jit_stress.c $(ALL_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

$(TEST_JIT_BENCHMARKS): test_jit_benchmarks.c $(ALL_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

# Test execution targets

# Run original Phase 8 tests
test-original: test-bytecode test-e2e

# Run bytecode generation tests
test-bytecode: $(TEST_BYTECODE)
	@echo "========================================="
	@echo "Running Bytecode Generation Tests"
	@echo "========================================="
	./$(TEST_BYTECODE)

# Run E2E pipeline tests
test-e2e: $(TEST_E2E)
	@echo "========================================="
	@echo "Running E2E Pipeline Tests"
	@echo "========================================="
	./$(TEST_E2E)

# Run JIT basic functionality tests
test-jit-basic: $(TEST_JIT_BASIC)
	@echo "========================================="
	@echo "Running JIT Basic Functionality Tests"
	@echo "========================================="
	./$(TEST_JIT_BASIC)

# Run JIT correctness verification tests
test-jit-correctness: $(TEST_JIT_CORRECTNESS)
	@echo "========================================="
	@echo "Running JIT Correctness Verification Tests"
	@echo "========================================="
	./$(TEST_JIT_CORRECTNESS)

# Run JIT performance tests
test-jit-performance: $(TEST_JIT_PERFORMANCE)
	@echo "========================================="
	@echo "Running JIT Performance Tests"
	@echo "========================================="
	./$(TEST_JIT_PERFORMANCE)

# Run JIT integration tests
test-jit-integration: $(TEST_JIT_INTEGRATION)
	@echo "========================================="
	@echo "Running JIT Integration Tests"
	@echo "========================================="
	./$(TEST_JIT_INTEGRATION)

# Run JIT stress tests
test-jit-stress: $(TEST_JIT_STRESS)
	@echo "========================================="
	@echo "Running JIT Stress Tests"
	@echo "========================================="
	./$(TEST_JIT_STRESS)

# Run JIT benchmarks
test-jit-benchmarks: $(TEST_JIT_BENCHMARKS)
	@echo "========================================="
	@echo "Running JIT Performance Benchmarks"
	@echo "========================================="
	./$(TEST_JIT_BENCHMARKS)

# Run all JIT tests
test-jit: $(JIT_TESTS)
	@echo "========================================="
	@echo "Running All JIT Integration Tests"
	@echo "========================================="
	@echo "1. Basic Functionality Tests"
	@./$(TEST_JIT_BASIC)
	@echo ""
	@echo "2. Correctness Verification Tests"
	@./$(TEST_JIT_CORRECTNESS)
	@echo ""
	@echo "3. Performance Tests"
	@./$(TEST_JIT_PERFORMANCE)
	@echo ""
	@echo "4. Integration Tests"
	@./$(TEST_JIT_INTEGRATION)
	@echo ""
	@echo "5. Stress Tests"
	@./$(TEST_JIT_STRESS)
	@echo ""
	@echo "6. Performance Benchmarks"
	@./$(TEST_JIT_BENCHMARKS)
	@echo ""
	@echo "========================================="
	@echo "All JIT tests completed!"
	@echo "========================================="

# Run all tests (original + JIT)
test-all: test-original test-jit
	@echo "========================================="
	@echo "All Phase 8 Tests Completed Successfully!"
	@echo "========================================="

# Default test target (all tests)
test: test-all

# Clean build artifacts
clean:
	rm -f $(ALL_TESTS) *.o

# Help
help:
	@echo "Available targets:"
	@echo "  all                - Build all tests"
	@echo "  test               - Run all tests (original + JIT)"
	@echo "  test-all           - Run all tests (original + JIT)"
	@echo "  test-original      - Run original Phase 8 tests only"
	@echo "  test-jit           - Run all JIT integration tests"
	@echo ""
	@echo "Original Phase 8 tests:"
	@echo "  test-bytecode      - Run bytecode generation tests"
	@echo "  test-e2e           - Run E2E pipeline tests"
	@echo ""
	@echo "JIT Integration tests:"
	@echo "  test-jit-basic     - Run JIT basic functionality tests (10 tests)"
	@echo "  test-jit-correctness - Run JIT correctness verification tests (10 tests)"
	@echo "  test-jit-performance - Run JIT performance tests (8 tests)"
	@echo "  test-jit-integration - Run JIT integration tests (10 tests)"
	@echo "  test-jit-stress    - Run JIT stress tests (8 tests)"
	@echo "  test-jit-benchmarks - Run JIT performance benchmarks (8 benchmarks)"
	@echo ""
	@echo "Total JIT tests: 54 comprehensive tests + benchmarks"
	@echo ""
	@echo "  clean              - Remove build artifacts"
	@echo "  help               - Show this help message"

# Test count summary
test-count:
	@echo "Test Count Summary:"
	@echo "  Original Phase 8 tests: 2"
	@echo "  JIT Basic tests: 10"
	@echo "  JIT Correctness tests: 10"
	@echo "  JIT Performance tests: 8"
	@echo "  JIT Integration tests: 10"
	@echo "  JIT Stress tests: 8"
	@echo "  JIT Benchmarks: 8"
	@echo "  Total JIT tests: 54"
	@echo "  Grand total: 56 tests + benchmarks"
