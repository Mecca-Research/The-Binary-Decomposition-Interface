# Makefile for ML Primitives

CC = gcc
CFLAGS = -std=gnu2x -Wall -Wextra -Wpedantic -O2 -march=native -I../..
LDFLAGS = -lm

# Source files
LINEAR_SRCS = linear/linear_regression.c
TREE_SRCS = tree/decision_tree.c
SVM_SRCS = kernel/svm.c
KMEANS_SRCS = clustering/kmeans.c

ML_SRCS = $(LINEAR_SRCS) $(TREE_SRCS) $(SVM_SRCS) $(KMEANS_SRCS)
ML_OBJS = $(ML_SRCS:.c=.o)

# Library
LIBML_BASE = libml_base.a

# Test executables
TESTS = ../tests/ml_linear_test \
        ../tests/ml_tree_test \
        ../tests/ml_svm_test \
        ../tests/ml_kmeans_test

.PHONY: all clean test

all: $(LIBML_BASE)

$(LIBML_BASE): $(ML_OBJS)
	ar rcs $@ $^
	@echo "Built ML base library: $(LIBML_BASE)"

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(ML_OBJS) $(LIBML_BASE)
	@echo "Cleaned ML base library"

test: $(TESTS)
	@echo "Running ML tests..."
	@for test in $(TESTS); do \
		if [ -f $$test ]; then \
			echo "Running $$test..."; \
			$$test || exit 1; \
		fi \
	done

.PHONY: help
help:
	@echo "ML Primitives Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build ML base library"
	@echo "  clean   - Remove build artifacts"
	@echo "  test    - Run all ML tests"
	@echo "  help    - Show this help message"
