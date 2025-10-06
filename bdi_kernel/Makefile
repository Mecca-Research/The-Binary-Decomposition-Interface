# BDI Kernel Makefile - C23 Standard
# Phase 1: C23 Foundation

# Compiler Configuration
CC := gcc
CFLAGS := -std=c2x -Wall -Wextra -Wpedantic -Werror

# C23 Feature Flags
# Note: GCC 12.2 has partial C23 support, full support in GCC 14+
CFLAGS += -Wno-unknown-pragmas

# Optimization Flags
CFLAGS += -O2 -march=native -mtune=native

# Include Paths
CFLAGS += -I. -Ikernel -Idevice -Ibackend -Ifs -Istorage -Iusb -Imath

# Debug Flags (optional)
DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CFLAGS += -g -DDEBUG
else
	CFLAGS += -DNDEBUG
endif

# Source Files
KERNEL_SRCS := kernel/graph.c kernel/ham.c kernel/motif.c kernel/integration.c kernel/main.c kernel/hash.c \
	kernel/memory.c kernel/pmm.c kernel/vmm.c \
	kernel/scheduler.c kernel/task.c kernel/smp.c kernel/ipi.c \
	kernel/ipc.c kernel/shm.c kernel/pipe.c kernel/socket.c
DEVICE_SRCS := device/device.c
BACKEND_SRCS := backend/gpu_backend.c backend/fpga_backend.c backend/bpu_device.c
BOOT_SRCS := boot/main.c
MATH_SRCS := math/smart_number.c math/mbh_arithmetic.c math/precision.c
PROCESS_SRCS := process/process_manager.c
SCHEDULER_SRCS := scheduler/scheduler.c
FS_SRCS := fs/fs_main.c fs/fs_bcache.c fs/fs_log.c fs/vfs/vfs.c fs/ext2/ext2.c fs/fat32/fat32.c
STORAGE_SRCS := storage/nvme/nvme.c storage/nvme/nvme_admin.c storage/nvme/nvme_io.c \
	storage/ahci/ahci.c storage/ahci/sata.c
USB_SRCS := usb/xhci/xhci.c usb/xhci/xhci_cmd.c usb/xhci/xhci_ring.c \
	usb/hid/hid_keyboard.c usb/hid/hid_mouse.c
SYSCALL_SRCS := syscalls/aeon_api.c
USERLAND_SRCS := userland/bdi_shell.c
DRIVER_SRCS := drivers/block_device.c drivers/ramdisk.c

ALL_SRCS := $(KERNEL_SRCS) $(DEVICE_SRCS) $(BACKEND_SRCS) $(BOOT_SRCS) \
	$(MATH_SRCS) $(PROCESS_SRCS) $(SCHEDULER_SRCS) $(FS_SRCS) \
	$(STORAGE_SRCS) $(USB_SRCS) $(SYSCALL_SRCS) $(USERLAND_SRCS) \
	$(DRIVER_SRCS)

# Object Files
OBJS := $(ALL_SRCS:.c=.o)

# Output Binary
TARGET := bdi_kernel

# Build Rules
.PHONY: all clean test info

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Linking $@..."
	$(CC) $(CFLAGS) -o $@ $^
	@echo "Build complete: $@"

%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning build artifacts..."
	rm -f $(OBJS) $(TARGET)
	rm -f **/*.o
	@echo "Clean complete"

test: $(TARGET)
	@echo "Running tests..."
	./$(TARGET) --test

info:
	@echo "BDI Kernel Build Information"
	@echo "============================"
	@echo "Compiler: $(CC)"
	@echo "Flags: $(CFLAGS)"
	@echo "Sources: $(words $(ALL_SRCS)) files"
	@echo "Target: $(TARGET)"

# Phase 5: Storage I/O Fast Paths
DRIVER_OBJS = drivers/nvme.o drivers/ahci.o drivers/xhci.o drivers/usb_hid.o
FS_OBJS = fs/fat32.o fs/ext2.o

OBJS += $(DRIVER_OBJS) $(FS_OBJS)

# ============================================================================
# Testing Infrastructure - Item 8
# ============================================================================

# Test directories
TEST_DIR := tests
STRESS_TEST_DIR := $(TEST_DIR)/stress
REGRESSION_TEST_DIR := $(TEST_DIR)/regression
PROFILING_DIR := profiling

# Test sources
TEST_RUNNER_SRC := $(TEST_DIR)/test_runner.c
STRESS_TEST_SRCS := $(wildcard $(STRESS_TEST_DIR)/*.c)
REGRESSION_TEST_SRCS := $(wildcard $(REGRESSION_TEST_DIR)/*.c)
PROFILING_SRCS := $(wildcard $(PROFILING_DIR)/*.c)

# Test binaries
TEST_RUNNER := test_runner
STRESS_TESTS := $(patsubst $(STRESS_TEST_DIR)/%.c,$(STRESS_TEST_DIR)/%,$(STRESS_TEST_SRCS))
REGRESSION_TESTS := $(patsubst $(REGRESSION_TEST_DIR)/%.c,$(REGRESSION_TEST_DIR)/%,$(REGRESSION_TEST_SRCS))

# Test flags
TEST_CFLAGS := $(CFLAGS) -I$(TEST_DIR) -I$(PROFILING_DIR)
TEST_LDFLAGS := $(LDFLAGS) -lpthread

# ============================================================================
# Test Targets
# ============================================================================

.PHONY: test test-unit test-integration test-stress test-regression test-performance
.PHONY: test-all test-clean build-tests

# Build all tests
build-tests: $(TEST_RUNNER) $(STRESS_TESTS) $(REGRESSION_TESTS)
	@echo "==> All tests built successfully"

# Build test runner
$(TEST_RUNNER): $(TEST_RUNNER_SRC) $(PROFILING_SRCS)
	@echo "==> Building test runner..."
	$(CC) $(TEST_CFLAGS) $(TEST_LDFLAGS) -o $@ $^

# Build stress tests
$(STRESS_TEST_DIR)/%: $(STRESS_TEST_DIR)/%.c
	@echo "==> Building stress test: $@"
	$(CC) $(TEST_CFLAGS) $(TEST_LDFLAGS) -o $@ $<

# Build regression tests
$(REGRESSION_TEST_DIR)/%: $(REGRESSION_TEST_DIR)/%.c
	@echo "==> Building regression test: $@"
	$(CC) $(TEST_CFLAGS) $(TEST_LDFLAGS) -o $@ $<

# Run all tests
test: build-tests
	@echo "==> Running all tests..."
	./$(TEST_RUNNER) --category=all --verbose

# Run unit tests
test-unit: build-tests
	@echo "==> Running unit tests..."
	./$(TEST_RUNNER) --category=unit --verbose

# Run integration tests
test-integration: build-tests
	@echo "==> Running integration tests..."
	./$(TEST_RUNNER) --category=integration --verbose

# Run stress tests
test-stress: build-tests
	@echo "==> Running stress tests..."
	./$(TEST_RUNNER) --category=stress --duration=60

# Run regression tests
test-regression: build-tests
	@echo "==> Running regression tests..."
	./$(TEST_RUNNER) --category=regression --verbose

# Run performance tests
test-performance: build-tests
	@echo "==> Running performance tests..."
	./$(TEST_RUNNER) --category=performance --output=json

# Run all test categories
test-all: test-unit test-integration test-regression test-performance
	@echo "==> All test categories completed"

# Clean test artifacts
test-clean:
	@echo "==> Cleaning test artifacts..."
	@rm -f $(TEST_RUNNER) $(STRESS_TESTS) $(REGRESSION_TESTS)
	@rm -rf test_results/
	@rm -f *.json *.csv
	@find $(TEST_DIR) -name "*.o" -delete

# ============================================================================
# Stress Test Targets (Individual)
# ============================================================================

.PHONY: stress-memory stress-scheduler stress-process

stress-memory: $(STRESS_TEST_DIR)/memory_stress
	@echo "==> Running memory stress test (60 seconds)..."
	./$(STRESS_TEST_DIR)/memory_stress 60

stress-scheduler: $(STRESS_TEST_DIR)/scheduler_stress
	@echo "==> Running scheduler stress test (60 seconds)..."
	./$(STRESS_TEST_DIR)/scheduler_stress 60

stress-process: $(STRESS_TEST_DIR)/process_stress
	@echo "==> Running process stress test (60 seconds)..."
	./$(STRESS_TEST_DIR)/process_stress 60

# ============================================================================
# Profiling Targets
# ============================================================================

.PHONY: profile profile-report

profile: build-tests
	@echo "==> Running tests with profiling enabled..."
	./$(TEST_RUNNER) --category=performance --verbose
	@echo "==> Profiling data collected"

profile-report:
	@echo "==> Generating profiling report..."
	@if [ -f profiler_output.json ]; then \
		cat profiler_output.json; \
	else \
		echo "No profiling data found. Run 'make profile' first."; \
	fi

# ============================================================================
# Coverage Targets
# ============================================================================

.PHONY: coverage coverage-report coverage-clean

coverage:
	@echo "==> Building with coverage instrumentation..."
	$(MAKE) clean
	$(MAKE) BUILD_MODE=debug CFLAGS="--coverage -fprofile-arcs -ftest-coverage -O0 -g" \
		LDFLAGS="--coverage -lgcov" all build-tests
	@echo "==> Running tests with coverage..."
	./$(TEST_RUNNER) --category=all || true
	@echo "==> Generating coverage report..."
	lcov --capture --directory . --output-file coverage.info
	lcov --remove coverage.info '/usr/*' --output-file coverage.info
	genhtml coverage.info --output-directory coverage_html
	@echo "==> Coverage report generated in coverage_html/"

coverage-report:
	@if [ -f coverage.info ]; then \
		lcov --list coverage.info; \
	else \
		echo "No coverage data found. Run 'make coverage' first."; \
	fi

coverage-clean:
	@echo "==> Cleaning coverage data..."
	@rm -f coverage.info
	@rm -rf coverage_html/
	@find . -name "*.gcda" -delete
	@find . -name "*.gcno" -delete

# ============================================================================
# CI/CD Helper Targets
# ============================================================================

.PHONY: ci-build ci-test ci-sanitizers ci-coverage

ci-build:
	@echo "==> CI: Building kernel..."
	$(MAKE) BUILD_MODE=release all

ci-test: build-tests
	@echo "==> CI: Running tests..."
	./$(TEST_RUNNER) --category=all --output=json > test_results.json || true

ci-sanitizers:
	@echo "==> CI: Building with sanitizers..."
	$(MAKE) clean
	$(MAKE) BUILD_MODE=debug CFLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g" \
		LDFLAGS="-fsanitize=address,undefined" all build-tests
	@echo "==> CI: Running tests with sanitizers..."
	./$(TEST_RUNNER) --category=unit --verbose

ci-coverage: coverage
	@echo "==> CI: Coverage build complete"

# ============================================================================
# Help Target Update
# ============================================================================

help: info
	@echo ""
	@echo "Testing Targets:"
	@echo "  make test                   - Run all tests"
	@echo "  make test-unit              - Run unit tests"
	@echo "  make test-integration       - Run integration tests"
	@echo "  make test-stress            - Run stress tests"
	@echo "  make test-regression        - Run regression tests"
	@echo "  make test-performance       - Run performance tests"
	@echo "  make test-all               - Run all test categories"
	@echo "  make test-clean             - Clean test artifacts"
	@echo ""
	@echo "Stress Test Targets:"
	@echo "  make stress-memory          - Run memory stress test"
	@echo "  make stress-scheduler       - Run scheduler stress test"
	@echo "  make stress-process         - Run process stress test"
	@echo ""
	@echo "Profiling Targets:"
	@echo "  make profile                - Run tests with profiling"
	@echo "  make profile-report         - Show profiling report"
	@echo ""
	@echo "Coverage Targets:"
	@echo "  make coverage               - Generate coverage report"
	@echo "  make coverage-report        - Show coverage summary"
	@echo "  make coverage-clean         - Clean coverage data"
	@echo ""
	@echo "CI/CD Targets:"
	@echo "  make ci-build               - CI build"
	@echo "  make ci-test                - CI test run"
	@echo "  make ci-sanitizers          - CI sanitizer tests"
	@echo "  make ci-coverage            - CI coverage build"

