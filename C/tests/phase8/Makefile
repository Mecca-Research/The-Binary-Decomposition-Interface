# Phase 8 Bytecode Generation Test Suite Makefile

CC = gcc
CFLAGS = -std=c2x -Wall -Wextra -Wpedantic -I../.. -I../../compiler/types -g -O2
LDFLAGS = -lm

# Source directories
COMPILER_DIR = ../../compiler
VM_DIR = ../../vm

# Compiler source files
CODEGEN_SOURCES = $(COMPILER_DIR)/codegen/codegen.c
AST_SOURCES = $(COMPILER_DIR)/ast/bci_ast.c
TYPE_SOURCES = $(COMPILER_DIR)/types/bci_types.c
SYMBOL_SOURCES = $(COMPILER_DIR)/semantic_analyzer/bci_symbol.c

# VM source files
VM_SOURCES = $(VM_DIR)/bci_chunk.c $(VM_DIR)/bci_vm.c

# All source files
ALL_SOURCES = $(CODEGEN_SOURCES) $(AST_SOURCES) $(TYPE_SOURCES) $(SYMBOL_SOURCES) $(VM_SOURCES)

# Test executables
TESTS = test_bytecode_generation

.PHONY: all clean run asan valgrind

all: $(TESTS)

test_bytecode_generation: test_bytecode_generation.c $(ALL_SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

run: all
	@echo "Running Phase 8 Bytecode Generation Tests..."
	@echo "=============================================="
	@./test_bytecode_generation

clean:
	rm -f $(TESTS) *.o

valgrind: test_bytecode_generation
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./test_bytecode_generation

asan: CFLAGS += -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
asan: LDFLAGS += -fsanitize=address -fsanitize=undefined
asan: clean all
	@echo "Running with Address Sanitizer..."
	@./test_bytecode_generation
