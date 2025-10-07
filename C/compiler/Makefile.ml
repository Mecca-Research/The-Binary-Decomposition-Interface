# Master Makefile for ML Primitives Integration

CC = gcc
CFLAGS = -std=gnu2x -Wall -Wextra -Wpedantic -O2 -march=native -I..
LDFLAGS = -lm

# Directories
AIBASE_DIR = AIBase
CODEGEN_DIR = CodeGen
VM_DIR = VM
TESTS_DIR = tests

# Source files
ML_BASE_SRCS = $(AIBASE_DIR)/linear/linear_regression.c \
               $(AIBASE_DIR)/tree/decision_tree.c \
               $(AIBASE_DIR)/kernel/svm.c \
               $(AIBASE_DIR)/clustering/kmeans.c

ML_CODEGEN_SRCS = $(CODEGEN_DIR)/ml_codegen.c
ML_OPS_SRCS = $(VM_DIR)/ml_ops.c

ML_ALL_SRCS = $(ML_BASE_SRCS) $(ML_CODEGEN_SRCS) $(ML_OPS_SRCS)
ML_ALL_OBJS = $(ML_ALL_SRCS:.c=.o)

# Libraries
LIBML = libml.a

# Test executables
ML_TESTS = $(TESTS_DIR)/ml_linear_test \
           $(TESTS_DIR)/ml_tree_test \
           $(TESTS_DIR)/ml_svm_test \
           $(TESTS_DIR)/ml_kmeans_test \
           $(TESTS_DIR)/ml_vm_test

.PHONY: all clean test install

all: $(LIBML)

$(LIBML): $(ML_ALL_OBJS)
	ar rcs $@ $^
	@echo "✓ Built complete ML library: $(LIBML)"

%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c -o $@ $<

# Build individual tests
$(TESTS_DIR)/ml_linear_test: $(TESTS_DIR)/ml_linear_test.c $(LIBML)
	@echo "Building linear regression test..."
	$(CC) $(CFLAGS) -o $@ $< $(LIBML) $(LDFLAGS)

$(TESTS_DIR)/ml_tree_test: $(TESTS_DIR)/ml_tree_test.c $(LIBML)
	@echo "Building decision tree test..."
	$(CC) $(CFLAGS) -o $@ $< $(LIBML) $(LDFLAGS)

$(TESTS_DIR)/ml_svm_test: $(TESTS_DIR)/ml_svm_test.c $(LIBML)
	@echo "Building SVM test..."
	$(CC) $(CFLAGS) -o $@ $< $(LIBML) $(LDFLAGS)

$(TESTS_DIR)/ml_kmeans_test: $(TESTS_DIR)/ml_kmeans_test.c $(LIBML)
	@echo "Building K-means test..."
	$(CC) $(CFLAGS) -o $@ $< $(LIBML) $(LDFLAGS)

$(TESTS_DIR)/ml_vm_test: $(TESTS_DIR)/ml_vm_test.c $(LIBML)
	@echo "Building VM integration test..."
	$(CC) $(CFLAGS) -o $@ $< $(LIBML) -L../../vm -lbci_vm $(LDFLAGS)

test: $(ML_TESTS)
	@echo ""
	@echo "=========================================="
	@echo "Running ML Primitives Test Suite"
	@echo "=========================================="
	@echo ""
	@for test in $(ML_TESTS); do \
		if [ -f $$test ]; then \
			echo ">>> Running $$test..."; \
			$$test || exit 1; \
			echo ""; \
		else \
			echo "Warning: $$test not found"; \
		fi \
	done
	@echo "=========================================="
	@echo "✓ All ML tests passed!"
	@echo "=========================================="

clean:
	rm -f $(ML_ALL_OBJS) $(LIBML) $(ML_TESTS)
	@echo "✓ Cleaned ML build artifacts"

install: $(LIBML)
	@echo "Installing ML library..."
	@mkdir -p ../lib
	cp $(LIBML) ../lib/
	@echo "✓ Installed $(LIBML) to ../lib/"

.PHONY: help
help:
	@echo "ML Primitives Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build complete ML library (libml.a)"
	@echo "  test     - Build and run all ML tests"
	@echo "  clean    - Remove all build artifacts"
	@echo "  install  - Install library to ../lib/"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Components:"
	@echo "  - Linear Regression (AIBase/linear/)"
	@echo "  - Decision Tree (AIBase/tree/)"
	@echo "  - SVM (AIBase/kernel/)"
	@echo "  - K-means (AIBase/clustering/)"
	@echo "  - CodeGen Integration (CodeGen/ml_codegen.c)"
	@echo "  - VM Operations (VM/ml_ops.c)"
