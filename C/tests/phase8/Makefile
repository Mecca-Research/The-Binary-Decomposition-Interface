# ===================================================================
# Makefile for Phase 8 Tests
# ===================================================================

CC = gcc
CFLAGS = -std=c2x -Wall -Wextra -Werror -O2 -g
INCLUDES = -I../.. -I../../compiler -I../../vm
LDFLAGS = -lm

# Source directories
COMPILER_DIR = ../../compiler
VM_DIR = ../../vm
GC_DIR = ../../vm/gc

# Source files
LEXER_SRC = $(COMPILER_DIR)/lexer/bci_lexer.c
PARSER_SRC = $(COMPILER_DIR)/parser/bci_parser.c
AST_SRC = $(COMPILER_DIR)/ast/bci_ast.c
CODEGEN_SRC = $(COMPILER_DIR)/codegen/codegen.c
CHUNK_SRC = $(VM_DIR)/bci_chunk.c
VM_SRC = $(VM_DIR)/bci_vm.c $(VM_DIR)/vm.c
GC_SRC = $(GC_DIR)/mark_sweep.c $(GC_DIR)/generational_gc.c
PIPELINE_SRC = ../../bdi_pipeline.c

# All source files
ALL_SRC = $(LEXER_SRC) $(PARSER_SRC) $(AST_SRC) $(CODEGEN_SRC) \
          $(CHUNK_SRC) $(VM_SRC) $(GC_SRC) $(PIPELINE_SRC)

# Test executables
TEST_BYTECODE = test_bytecode_generation
TEST_E2E = test_e2e_pipeline

.PHONY: all clean test test-bytecode test-e2e

all: $(TEST_BYTECODE) $(TEST_E2E)

# Build bytecode generation tests
$(TEST_BYTECODE): test_bytecode_generation.c $(ALL_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

# Build E2E pipeline tests
$(TEST_E2E): test_e2e_pipeline.c $(ALL_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

# Run all tests
test: test-bytecode test-e2e

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

# Clean build artifacts
clean:
	rm -f $(TEST_BYTECODE) $(TEST_E2E) *.o

# Help
help:
	@echo "Available targets:"
	@echo "  all           - Build all tests"
	@echo "  test          - Run all tests"
	@echo "  test-bytecode - Run bytecode generation tests"
	@echo "  test-e2e      - Run E2E pipeline tests"
	@echo "  clean         - Remove build artifacts"
	@echo "  help          - Show this help message"
