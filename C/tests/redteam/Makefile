
# Makefile for BDI Kernel Red-Team Memory Test Suite
# Comprehensive testing with sanitizer support

CC = clang
CFLAGS = -std=c2x -Wall -Wextra -Werror -pedantic -O2 -g
INCLUDES = -I../../.. -I../../../kernel
LDFLAGS = -pthread -lm

# Directories
BUILD_DIR = build
ASAN_DIR = $(BUILD_DIR)/asan
UBSAN_DIR = $(BUILD_DIR)/ubsan
TSAN_DIR = $(BUILD_DIR)/tsan
DEBUG_DIR = $(BUILD_DIR)/debug

# Common sources
COMMON_SRCS = common/redteam_harness.c \
              common/fault_injection.c \
              common/fuzzing_utils.c \
              common/thread_utils.c

COMMON_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(COMMON_SRCS))

# Test sources
TEST_SRCS = allocator/test_allocator_redteam.c \
            numa/test_numa_arena_redteam.c \
            hugepage/test_hugepage_redteam.c \
            pmm/test_pmm_redteam.c \
            vmm/test_vmm_redteam.c \
            ham/test_ham_redteam.c \
            instrumentation/test_instrumentation.c

# Test executables
TESTS = $(patsubst %.c,$(BUILD_DIR)/%,$(TEST_SRCS))
ASAN_TESTS = $(patsubst %.c,$(ASAN_DIR)/%,$(TEST_SRCS))
UBSAN_TESTS = $(patsubst %.c,$(UBSAN_DIR)/%,$(TEST_SRCS))
TSAN_TESTS = $(patsubst %.c,$(TSAN_DIR)/%,$(TEST_SRCS))
DEBUG_TESTS = $(patsubst %.c,$(DEBUG_DIR)/%,$(TEST_SRCS))

# Sanitizer flags
ASAN_FLAGS = -fsanitize=address -fno-omit-frame-pointer
UBSAN_FLAGS = -fsanitize=undefined -fno-omit-frame-pointer
TSAN_FLAGS = -fsanitize=thread -fno-omit-frame-pointer
DEBUG_FLAGS = -O0 -DDEBUG

.PHONY: all clean asan ubsan tsan debug sanitizers test help

# Default target
all: $(BUILD_DIR) $(TESTS)

# Create build directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)/common
	mkdir -p $(BUILD_DIR)/allocator
	mkdir -p $(BUILD_DIR)/numa
	mkdir -p $(BUILD_DIR)/hugepage
	mkdir -p $(BUILD_DIR)/pmm
	mkdir -p $(BUILD_DIR)/vmm
	mkdir -p $(BUILD_DIR)/ham
	mkdir -p $(BUILD_DIR)/instrumentation

$(ASAN_DIR):
	mkdir -p $(ASAN_DIR)/common
	mkdir -p $(ASAN_DIR)/allocator
	mkdir -p $(ASAN_DIR)/numa
	mkdir -p $(ASAN_DIR)/hugepage
	mkdir -p $(ASAN_DIR)/pmm
	mkdir -p $(ASAN_DIR)/vmm
	mkdir -p $(ASAN_DIR)/ham
	mkdir -p $(ASAN_DIR)/instrumentation

$(UBSAN_DIR):
	mkdir -p $(UBSAN_DIR)/common
	mkdir -p $(UBSAN_DIR)/allocator
	mkdir -p $(UBSAN_DIR)/numa
	mkdir -p $(UBSAN_DIR)/hugepage
	mkdir -p $(UBSAN_DIR)/pmm
	mkdir -p $(UBSAN_DIR)/vmm
	mkdir -p $(UBSAN_DIR)/ham
	mkdir -p $(UBSAN_DIR)/instrumentation

$(TSAN_DIR):
	mkdir -p $(TSAN_DIR)/common
	mkdir -p $(TSAN_DIR)/allocator
	mkdir -p $(TSAN_DIR)/numa
	mkdir -p $(TSAN_DIR)/hugepage
	mkdir -p $(TSAN_DIR)/pmm
	mkdir -p $(TSAN_DIR)/vmm
	mkdir -p $(TSAN_DIR)/ham
	mkdir -p $(TSAN_DIR)/instrumentation

$(DEBUG_DIR):
	mkdir -p $(DEBUG_DIR)/common
	mkdir -p $(DEBUG_DIR)/allocator
	mkdir -p $(DEBUG_DIR)/numa
	mkdir -p $(DEBUG_DIR)/hugepage
	mkdir -p $(DEBUG_DIR)/pmm
	mkdir -p $(DEBUG_DIR)/vmm
	mkdir -p $(DEBUG_DIR)/ham
	mkdir -p $(DEBUG_DIR)/instrumentation

# Build common objects
$(BUILD_DIR)/common/%.o: common/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Build standard tests
$(BUILD_DIR)/%: %.c $(COMMON_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(COMMON_OBJS) $(LDFLAGS) -o $@

# Build ASAN tests
$(ASAN_DIR)/common/%.o: common/%.c | $(ASAN_DIR)
	$(CC) $(CFLAGS) $(ASAN_FLAGS) $(INCLUDES) -c $< -o $@

$(ASAN_DIR)/%: %.c $(patsubst $(BUILD_DIR)/%,$(ASAN_DIR)/%,$(COMMON_OBJS)) | $(ASAN_DIR)
	$(CC) $(CFLAGS) $(ASAN_FLAGS) $(INCLUDES) $< $(patsubst $(BUILD_DIR)/%,$(ASAN_DIR)/%,$(COMMON_OBJS)) $(LDFLAGS) -o $@

# Build UBSAN tests
$(UBSAN_DIR)/common/%.o: common/%.c | $(UBSAN_DIR)
	$(CC) $(CFLAGS) $(UBSAN_FLAGS) $(INCLUDES) -c $< -o $@

$(UBSAN_DIR)/%: %.c $(patsubst $(BUILD_DIR)/%,$(UBSAN_DIR)/%,$(COMMON_OBJS)) | $(UBSAN_DIR)
	$(CC) $(CFLAGS) $(UBSAN_FLAGS) $(INCLUDES) $< $(patsubst $(BUILD_DIR)/%,$(UBSAN_DIR)/%,$(COMMON_OBJS)) $(LDFLAGS) -o $@

# Build TSAN tests
$(TSAN_DIR)/common/%.o: common/%.c | $(TSAN_DIR)
	$(CC) $(CFLAGS) $(TSAN_FLAGS) $(INCLUDES) -c $< -o $@

$(TSAN_DIR)/%: %.c $(patsubst $(BUILD_DIR)/%,$(TSAN_DIR)/%,$(COMMON_OBJS)) | $(TSAN_DIR)
	$(CC) $(CFLAGS) $(TSAN_FLAGS) $(INCLUDES) $< $(patsubst $(BUILD_DIR)/%,$(TSAN_DIR)/%,$(COMMON_OBJS)) $(LDFLAGS) -o $@

# Build debug tests
$(DEBUG_DIR)/common/%.o: common/%.c | $(DEBUG_DIR)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(INCLUDES) -c $< -o $@

$(DEBUG_DIR)/%: %.c $(patsubst $(BUILD_DIR)/%,$(DEBUG_DIR)/%,$(COMMON_OBJS)) | $(DEBUG_DIR)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(INCLUDES) $< $(patsubst $(BUILD_DIR)/%,$(DEBUG_DIR)/%,$(COMMON_OBJS)) $(LDFLAGS) -o $@

# Sanitizer targets
asan: $(ASAN_DIR) $(ASAN_TESTS)
	@echo "Built ASAN tests in $(ASAN_DIR)"

ubsan: $(UBSAN_DIR) $(UBSAN_TESTS)
	@echo "Built UBSAN tests in $(UBSAN_DIR)"

tsan: $(TSAN_DIR) $(TSAN_TESTS)
	@echo "Built TSAN tests in $(TSAN_DIR)"

debug: $(DEBUG_DIR) $(DEBUG_TESTS)
	@echo "Built debug tests in $(DEBUG_DIR)"

sanitizers: asan ubsan tsan
	@echo "Built all sanitizer variants"

# Run tests
test: all
	@echo "Running red-team tests..."
	@./scripts/run_all_tests.sh

test-asan: asan
	@echo "Running ASAN tests..."
	@for test in $(ASAN_TESTS); do \
		echo "Running $$test..."; \
		$$test || exit 1; \
	done

test-ubsan: ubsan
	@echo "Running UBSAN tests..."
	@for test in $(UBSAN_TESTS); do \
		echo "Running $$test..."; \
		$$test || exit 1; \
	done

test-tsan: tsan
	@echo "Running TSAN tests..."
	@for test in $(TSAN_TESTS); do \
		echo "Running $$test..."; \
		$$test || exit 1; \
	done

test-all: test test-asan test-ubsan test-tsan
	@echo "All tests completed"

# Clean
clean:
	rm -rf $(BUILD_DIR)
	rm -f *.log

# Help
help:
	@echo "BDI Kernel Red-Team Memory Test Suite"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build standard tests (default)"
	@echo "  asan         - Build with Address Sanitizer"
	@echo "  ubsan        - Build with Undefined Behavior Sanitizer"
	@echo "  tsan         - Build with Thread Sanitizer"
	@echo "  debug        - Build with debug symbols and no optimization"
	@echo "  sanitizers   - Build all sanitizer variants"
	@echo "  test         - Run standard tests"
	@echo "  test-asan    - Run ASAN tests"
	@echo "  test-ubsan   - Run UBSAN tests"
	@echo "  test-tsan    - Run TSAN tests"
	@echo "  test-all     - Run all test variants"
	@echo "  clean        - Remove build artifacts"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make              # Build standard tests"
	@echo "  make asan         # Build with ASAN"
	@echo "  make test         # Run standard tests"
	@echo "  make test-all     # Run all test variants"
