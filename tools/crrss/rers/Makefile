
# Makefile for RERS (Runtime Error Replay System)

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -g
INCLUDES = -I.
LDFLAGS =

# Source files
SRCS = rers.c rers_replay.c rers_learning.c rers_patterns.c rers_integration.c
OBJS = $(SRCS:.c=.o)

# Test files
TEST_SRCS = tests/test_rers_main.c \
            tests/test_rers_replay.c \
            tests/test_rers_learning.c \
            tests/test_rers_patterns.c \
            tests/test_rers_integration.c

TEST_BINS = tests/test_rers_main \
            tests/test_rers_replay \
            tests/test_rers_learning \
            tests/test_rers_patterns \
            tests/test_rers_integration

# Library
LIB = librers.a

# Default target
all: $(LIB) tests

# Build library
$(LIB): $(OBJS)
	@echo "Creating RERS library..."
	ar rcs $@ $^
	@echo "✓ Library created: $@"

# Build object files
%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Build tests
tests: $(TEST_BINS)

tests/test_rers_main: tests/test_rers_main.c $(LIB)
	@echo "Building $@..."
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIB) -o $@ $(LDFLAGS)

tests/test_rers_replay: tests/test_rers_replay.c $(LIB)
	@echo "Building $@..."
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIB) -o $@ $(LDFLAGS)

tests/test_rers_learning: tests/test_rers_learning.c $(LIB)
	@echo "Building $@..."
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIB) -o $@ $(LDFLAGS)

tests/test_rers_patterns: tests/test_rers_patterns.c $(LIB)
	@echo "Building $@..."
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIB) -o $@ $(LDFLAGS)

tests/test_rers_integration: tests/test_rers_integration.c $(LIB)
	@echo "Building $@..."
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIB) -o $@ $(LDFLAGS)

# Run tests
test: tests
	@echo ""
	@echo "═══════════════════════════════════════════════════════════"
	@echo "  Running RERS Test Suite"
	@echo "═══════════════════════════════════════════════════════════"
	@echo ""
	@./tests/test_rers_main
	@./tests/test_rers_replay
	@./tests/test_rers_learning
	@./tests/test_rers_patterns
	@./tests/test_rers_integration
	@echo ""
	@echo "═══════════════════════════════════════════════════════════"
	@echo "  ✓ All RERS Tests Passed Successfully!"
	@echo "═══════════════════════════════════════════════════════════"
	@echo ""

# Clean
clean:
	@echo "Cleaning RERS build artifacts..."
	rm -f $(OBJS) $(LIB) $(TEST_BINS)
	@echo "✓ Clean complete"

# Help
help:
	@echo "RERS (Runtime Error Replay System) - Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build library and tests (default)"
	@echo "  $(LIB)     - Build RERS library"
	@echo "  tests      - Build all test programs"
	@echo "  test       - Build and run all tests"
	@echo "  clean      - Remove all build artifacts"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Components:"
	@echo "  - Error Replay Engine (segfaults, assertions, memory leaks, logic errors)"
	@echo "  - Active Learning System (hierarchical, priority-based)"
	@echo "  - Bug Pattern Database (in-memory, fast matching)"
	@echo "  - Integration Layer (coordinates MSM, STP, BPME, TDT profiles)"

.PHONY: all tests test clean help
