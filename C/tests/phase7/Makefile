# Phase 7 VM Test Suite Makefile

CC = gcc
CFLAGS = -std=c2x -Wall -Wextra -Wpedantic -I../.. -I../../compiler/types -g -O2
LDFLAGS = -lm

# VM and GC source files
VM_DIR = ../../vm
GC_DIR = ../../vm/gc

VM_SOURCES = $(VM_DIR)/bci_vm.c $(VM_DIR)/bci_chunk.c $(VM_DIR)/vm.c
GC_SOURCES = $(GC_DIR)/mark_sweep.c $(GC_DIR)/generational_gc.c

# Test executables
TESTS = test_vm_gc_integration

.PHONY: all clean run

all: $(TESTS)

test_vm_gc_integration: test_vm_gc_integration.c $(VM_SOURCES) $(GC_SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

run: all
	@echo "Running Phase 7 VM-GC Integration Tests..."
	@echo "=========================================="
	@./test_vm_gc_integration

clean:
	rm -f $(TESTS) *.o

valgrind: test_vm_gc_integration
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./test_vm_gc_integration

asan: CFLAGS += -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
asan: LDFLAGS += -fsanitize=address -fsanitize=undefined
asan: clean all
	@echo "Running with Address Sanitizer..."
	@./test_vm_gc_integration
