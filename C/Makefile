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

# Phase 3 source files
TYPES_EXT_SRCS = compiler/types/bci_types.c compiler/types/bci_types_extended.c
PARSER_EXT_SRCS = compiler/parser/bci_parser_extended.c compiler/ast/bci_ast.c compiler/ast/bci_ast_extended.c
SEMANTIC_EXT_SRCS = compiler/semantic_analyzer/bci_type_inference.c compiler/semantic_analyzer/bci_lifetime.c \
	            compiler/semantic_analyzer/bci_escape.c compiler/semantic_analyzer/bci_cfg.c

# Object files
BCI_OBJS = $(BCI_SRCS:.c=.o)
BTL_OBJS = $(BTL_SRCS:.c=.o)
TYPES_EXT_OBJS = $(TYPES_EXT_SRCS:.c=.o)
PARSER_EXT_OBJS = $(PARSER_EXT_SRCS:.c=.o)
SEMANTIC_EXT_OBJS = $(SEMANTIC_EXT_SRCS:.c=.o)

# Libraries
LIBBCI = libbci.a
LIBBTL = libbtl.a
LIBTYPES_EXT = libbci_types_extended.a
LIBPARSER_EXT = libbci_parser_extended.a
LIBSEMANTIC_EXT = libbci_semantic_extended.a

# Test executables
TESTS = tests/test_bci_arithmetic tests/test_bci_bitops tests/test_bci_conversion tests/test_bci_simd \
	tests/test_btl_isa tests/test_btl_regalloc tests/test_btl_scheduler tests/test_btl_peephole

# Phase 3 test executables
PHASE3_TESTS = tests/test_phase3_types tests/test_phase3_parser tests/test_phase3_semantic

# Benchmarks
BENCHMARKS = tests/benchmark_bci tests/benchmark_btl

.PHONY: all clean test benchmarks phase3 test_phase3

all: $(LIBBCI) $(LIBBTL) $(TESTS) $(BENCHMARKS)

phase3: $(LIBTYPES_EXT) $(LIBPARSER_EXT) $(LIBSEMANTIC_EXT) $(PHASE3_TESTS)

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

# Phase 3 libraries
$(LIBTYPES_EXT): $(TYPES_EXT_OBJS)
	ar rcs $@ $^

$(LIBPARSER_EXT): $(PARSER_EXT_OBJS)
	ar rcs $@ $^

$(LIBSEMANTIC_EXT): $(SEMANTIC_EXT_OBJS)
	ar rcs $@ $^

# Phase 3 tests
tests/test_phase3_types: tests/test_phase3_types.c $(LIBTYPES_EXT) $(LIBBCI)
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -lbci_types_extended -lbci $(LDFLAGS) -o $@

tests/test_phase3_parser: tests/test_phase3_parser.c $(LIBPARSER_EXT) $(LIBTYPES_EXT) $(LIBBCI)
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -lbci_parser_extended -lbci_types_extended -lbci $(LDFLAGS) -o $@

tests/test_phase3_semantic: tests/test_phase3_semantic.c $(LIBSEMANTIC_EXT) $(LIBPARSER_EXT) $(LIBTYPES_EXT) $(LIBBCI)
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -lbci_semantic_extended -lbci_parser_extended -lbci_types_extended -lbci $(LDFLAGS) -o $@

# Run Phase 3 tests
test_phase3: $(PHASE3_TESTS)
	@echo "Running Phase 3 Type System tests..."
	@./tests/test_phase3_types
	@echo ""
	@echo "Running Phase 3 Parser tests..."
	@./tests/test_phase3_parser
	@echo ""
	@echo "Running Phase 3 Semantic Analysis tests..."
	@./tests/test_phase3_semantic

clean:
	rm -f $(BCI_OBJS) $(BTL_OBJS) $(TYPES_EXT_OBJS) $(PARSER_EXT_OBJS) $(SEMANTIC_EXT_OBJS)
	rm -f $(LIBBCI) $(LIBBTL) $(LIBTYPES_EXT) $(LIBPARSER_EXT) $(LIBSEMANTIC_EXT)
	rm -f $(TESTS) $(PHASE3_TESTS) $(BENCHMARKS)
