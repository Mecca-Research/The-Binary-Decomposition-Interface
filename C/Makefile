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

# Object files
BCI_OBJS = $(BCI_SRCS:.c=.o)
BTL_OBJS = $(BTL_SRCS:.c=.o)

# Libraries
LIBBCI = libbci.a
LIBBTL = libbtl.a

# Test executables
TESTS = tests/test_bci_arithmetic tests/test_bci_bitops tests/test_bci_conversion tests/test_bci_simd \
        tests/test_btl_isa tests/test_btl_regalloc tests/test_btl_scheduler tests/test_btl_peephole

# Benchmarks
BENCHMARKS = tests/benchmark_bci tests/benchmark_btl

.PHONY: all clean test benchmarks

all: $(LIBBCI) $(LIBBTL) $(TESTS) $(BENCHMARKS)

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

clean:
	rm -f $(BCI_OBJS) $(BTL_OBJS) $(LIBBCI) $(LIBBTL) $(TESTS) $(BENCHMARKS)
