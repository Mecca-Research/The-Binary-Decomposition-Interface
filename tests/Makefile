
# Test Suite Makefile for BDI C23 Modernization

CC = gcc
CFLAGS = -std=c2x -Wall -Wextra -Wpedantic -I. -g
LDFLAGS = 

# Test executables
TESTS = test_nullptr test_attributes test_static_assert test_typeof

# All test sources
TEST_SOURCES = test_nullptr.c test_attributes.c test_static_assert.c test_typeof.c
FRAMEWORK_SOURCES = test_framework.c

.PHONY: all clean run

all: $(TESTS)

test_nullptr: test_nullptr.c test_framework.c test_framework.h
	$(CC) $(CFLAGS) -o $@ test_nullptr.c test_framework.c $(LDFLAGS)

test_attributes: test_attributes.c test_framework.c test_framework.h
	$(CC) $(CFLAGS) -o $@ test_attributes.c test_framework.c $(LDFLAGS)

test_static_assert: test_static_assert.c test_framework.c test_framework.h
	$(CC) $(CFLAGS) -o $@ test_static_assert.c test_framework.c $(LDFLAGS)

test_typeof: test_typeof.c test_framework.c test_framework.h
	$(CC) $(CFLAGS) -o $@ test_typeof.c test_framework.c $(LDFLAGS)

run: all
	@echo "Running test suite..."
	@for test in $(TESTS); do \
		echo ""; \
		echo "========================================"; \
		echo "Running $$test"; \
		echo "========================================"; \
		./$$test || exit 1; \
	done
	@echo ""
	@echo "All tests passed!"

clean:
	rm -f $(TESTS) *.o

