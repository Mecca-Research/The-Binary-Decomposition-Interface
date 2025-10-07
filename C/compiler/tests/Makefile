# Makefile for Bug Fix Tests
# Tests for RL Framework realloc fix and DSL parameter parsing fix

CC = gcc
CFLAGS = -std=c2x -Wall -Wextra -O2 -g -I.. -I../..
LDFLAGS = -lm

# Directories
RL_DIR = ../AIBase/reinforcement

# Source files for dependencies
RL_SOURCES = $(RL_DIR)/rl_framework.c $(RL_DIR)/q_learning.c

# Object files
RL_OBJECTS = $(RL_SOURCES:.c=.o)

# Test executables
TESTS = test_rl_realloc_fix test_dsl_parsing_fix

.PHONY: all clean test run-tests

all: $(TESTS)

# Build RL realloc fix test
test_rl_realloc_fix: test_rl_realloc_fix.o $(RL_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Build DSL parsing fix test (standalone - no external dependencies)
test_dsl_parsing_fix: test_dsl_parsing_fix.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Pattern rule for object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run all tests
test: run-tests

run-tests: $(TESTS)
	@echo "=========================================="
	@echo "Running Bug Fix Tests"
	@echo "=========================================="
	@echo ""
	@echo "Test 1: RL Framework Realloc Fix"
	@echo "------------------------------------------"
	@./test_rl_realloc_fix
	@echo ""
	@echo "Test 2: DSL Parameter Parsing Fix"
	@echo "------------------------------------------"
	@./test_dsl_parsing_fix
	@echo ""
	@echo "=========================================="
	@echo "All Tests Completed Successfully!"
	@echo "=========================================="

# Clean build artifacts
clean:
	rm -f *.o $(TESTS)
	rm -f $(RL_OBJECTS)
	@echo "Cleaned test build artifacts"

# Help
help:
	@echo "Bug Fix Tests Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all              - Build all tests (default)"
	@echo "  test             - Build and run all tests"
	@echo "  run-tests        - Run all tests"
	@echo "  clean            - Remove build artifacts"
	@echo "  help             - Show this help message"
	@echo ""
	@echo "Tests:"
	@echo "  test_rl_realloc_fix   - Test for RL framework realloc bug fix"
	@echo "  test_dsl_parsing_fix  - Test for DSL parameter parsing bug fix"
