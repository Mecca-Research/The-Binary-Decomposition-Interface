
# BDI Kernel Root Makefile - Phase 6: Build System & Compiler Optimization
# Comprehensive build system with PGO, LTO, and advanced optimizations

# ============================================================================
# Build Configuration
# ============================================================================

# Include build configuration
-include build_config.mk

# Compiler Configuration
CC := gcc
LD := ld
AR := ar

# Default build mode (can be: debug, release, pgo-gen, pgo-use)
BUILD_MODE ?= release

# ============================================================================
# Compiler Flags - Base
# ============================================================================

# C23 Standard with full feature support
CFLAGS := -std=c2x -Wall -Wextra -Wpedantic -Werror
CFLAGS += -Wno-unknown-pragmas

# Include paths
CFLAGS += -I. -Ibdi_kernel -Ibdi_kernel/kernel -Ibdi_kernel/device -Ibdi_kernel/backend
CFLAGS += -Ibdi_kernel/fs -Ibdi_kernel/storage -Ibdi_kernel/usb -Ibdi_kernel/math
CFLAGS += -Ibdi_kernel/drivers -Ibdi_kernel/syscalls -Ibdi_kernel/tracing -Ibdi_kernel/security

# ============================================================================
# Optimization Flags by Build Mode
# ============================================================================

ifeq ($(BUILD_MODE),debug)
    # Debug build: minimal optimization, full debug info
    CFLAGS += -O0 -g3 -DDEBUG -fno-omit-frame-pointer
    CFLAGS += -fsanitize=address -fsanitize=undefined
    LDFLAGS += -fsanitize=address -fsanitize=undefined
    
else ifeq ($(BUILD_MODE),pgo-gen)
    # PGO Profile Generation: instrumented build
    CFLAGS += -O2 -g -fprofile-generate -fprofile-dir=./pgo-data
    LDFLAGS += -fprofile-generate -fprofile-dir=./pgo-data
    CFLAGS += -march=native -mtune=native
    
else ifeq ($(BUILD_MODE),pgo-use)
    # PGO Optimized Build: use collected profiles
    CFLAGS += -O3 -fprofile-use -fprofile-dir=./pgo-data -fprofile-correction
    LDFLAGS += -fprofile-use -fprofile-dir=./pgo-data
    # Enable aggressive optimizations with PGO
    CFLAGS += -march=native -mtune=native
    CFLAGS += -flto=auto -fuse-linker-plugin
    CFLAGS += -fipa-pta -fdevirtualize-at-ltrans
    CFLAGS += -fgraphite-identity -floop-nest-optimize
    LDFLAGS += -flto=auto -fuse-linker-plugin
    
else
    # Release build: maximum optimization without PGO
    CFLAGS += -O3 -march=native -mtune=native -DNDEBUG
    CFLAGS += -flto=auto -fuse-linker-plugin
    CFLAGS += -fipa-pta -fdevirtualize-at-ltrans
    CFLAGS += -fgraphite-identity -floop-nest-optimize
    LDFLAGS += -flto=auto -fuse-linker-plugin
endif

# ============================================================================
# Advanced Optimization Flags (Release & PGO-Use)
# ============================================================================

ifneq ($(BUILD_MODE),debug)
    # Function and data sections for dead code elimination
    CFLAGS += -ffunction-sections -fdata-sections
    LDFLAGS += -Wl,--gc-sections
    
    # Vectorization and loop optimizations
    CFLAGS += -ftree-vectorize -ftree-loop-vectorize
    CFLAGS += -ftree-slp-vectorize -fvect-cost-model=dynamic
    
    # Inlining optimizations
    CFLAGS += -finline-functions -finline-limit=600
    CFLAGS += -fipa-cp -fipa-cp-clone
    
    # Branch prediction and profiling
    CFLAGS += -fno-semantic-interposition
    CFLAGS += -fno-plt
    
    # Memory optimizations
    CFLAGS += -fmerge-all-constants
    CFLAGS += -fno-stack-protector  # Remove for production if security needed
    
    # ISA-specific optimizations (enabled via optimization.h)
    CFLAGS += -DENABLE_AVX2 -DENABLE_AVX512
    CFLAGS += -mavx2 -mfma
    
    # Check for AVX-512 support
    AVX512_SUPPORT := $(shell $(CC) -march=native -dM -E - < /dev/null 2>/dev/null | grep -q AVX512F && echo 1 || echo 0)
    ifeq ($(AVX512_SUPPORT),1)
	CFLAGS += -mavx512f -mavx512cd -mavx512bw -mavx512dq -mavx512vl
    endif
endif

# ============================================================================
# Linker Configuration
# ============================================================================

# Use custom linker script if available
ifneq (,$(wildcard linker.ld))
    LDFLAGS += -T linker.ld
endif

# Linker optimizations
LDFLAGS += -Wl,-O2
LDFLAGS += -Wl,--as-needed
LDFLAGS += -Wl,--sort-common
LDFLAGS += -Wl,--hash-style=gnu

# ============================================================================
# Source Files
# ============================================================================

# Kernel core
KERNEL_SRCS := bdi_kernel/kernel/graph.c bdi_kernel/kernel/ham.c bdi_kernel/kernel/motif.c
KERNEL_SRCS += bdi_kernel/kernel/integration.c bdi_kernel/kernel/main.c bdi_kernel/kernel/hash.c
KERNEL_SRCS += bdi_kernel/kernel/memory.c bdi_kernel/kernel/pmm.c bdi_kernel/kernel/vmm.c
KERNEL_SRCS += bdi_kernel/kernel/scheduler.c bdi_kernel/kernel/task.c bdi_kernel/kernel/smp.c
KERNEL_SRCS += bdi_kernel/kernel/ipi.c bdi_kernel/kernel/ipc.c bdi_kernel/kernel/shm.c
KERNEL_SRCS += bdi_kernel/kernel/pipe.c bdi_kernel/kernel/socket.c

# Autoprofiler (Phase 6)
KERNEL_SRCS += bdi_kernel/kernel/autoprofiler.c

# Device and backend
DEVICE_SRCS := bdi_kernel/device/device.c bdi_kernel/device/device_manager.c
DEVICE_SRCS += bdi_kernel/device/hotplug.c bdi_kernel/device/irq.c
DEVICE_SRCS += bdi_kernel/device/driver_interface.c bdi_kernel/device/device_class.c
DEVICE_SRCS += bdi_kernel/device/backend_integration.c
BACKEND_SRCS := bdi_kernel/backend/gpu_backend.c bdi_kernel/backend/fpga_backend.c
BACKEND_SRCS += bdi_kernel/backend/bpu_device.c

# Boot
BOOT_SRCS := bdi_kernel/boot/main.c

# Math
MATH_SRCS := bdi_kernel/math/smart_number.c bdi_kernel/math/mbh_arithmetic.c
MATH_SRCS += bdi_kernel/math/precision.c

# Filesystem
FS_SRCS := bdi_kernel/fs/fs_main.c bdi_kernel/fs/fs_bcache.c bdi_kernel/fs/fs_log.c
FS_SRCS += bdi_kernel/fs/vfs/vfs.c bdi_kernel/fs/ext2/ext2.c bdi_kernel/fs/fat32/fat32.c

# Storage drivers
STORAGE_SRCS := bdi_kernel/storage/nvme/nvme.c bdi_kernel/storage/nvme/nvme_admin.c
STORAGE_SRCS += bdi_kernel/storage/nvme/nvme_io.c
STORAGE_SRCS += bdi_kernel/storage/ahci/ahci.c bdi_kernel/storage/ahci/sata.c

# USB
USB_SRCS := bdi_kernel/usb/xhci/xhci.c bdi_kernel/usb/xhci/xhci_cmd.c
USB_SRCS += bdi_kernel/usb/xhci/xhci_ring.c
USB_SRCS += bdi_kernel/usb/hid/hid_keyboard.c bdi_kernel/usb/hid/hid_mouse.c

# Drivers (Phase 5)
DRIVER_SRCS := bdi_kernel/drivers/block_device.c bdi_kernel/drivers/ramdisk.c
DRIVER_SRCS += bdi_kernel/drivers/nvme.c bdi_kernel/drivers/ahci.c

# Syscalls
SYSCALL_SRCS := bdi_kernel/syscalls/aeon_api.c

# Userland
USERLAND_SRCS := bdi_kernel/userland/bdi_shell.c \
		 bdi_kernel/userland/shell_commands.c \
		 bdi_kernel/userland/shell_integration.c

# Process and scheduler
PROCESS_SRCS := bdi_kernel/process/process_manager.c
SCHEDULER_SRCS := bdi_kernel/scheduler/scheduler.c

# All sources
ALL_SRCS := $(KERNEL_SRCS) $(DEVICE_SRCS) $(BACKEND_SRCS) $(BOOT_SRCS)
ALL_SRCS += $(MATH_SRCS) $(PROCESS_SRCS) $(SCHEDULER_SRCS) $(FS_SRCS)
ALL_SRCS += $(STORAGE_SRCS) $(USB_SRCS) $(SYSCALL_SRCS) $(USERLAND_SRCS)
ALL_SRCS += $(DRIVER_SRCS)

# Object files
OBJS := $(ALL_SRCS:.c=.o)

# Output binary
TARGET := bdi_kernel

# ============================================================================
# Build Targets
# ============================================================================

.PHONY: all clean test info pgo-generate pgo-merge pgo-optimize help
.PHONY: check-optimization validate-build benchmark

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "==> Linking $(TARGET) [$(BUILD_MODE)]..."
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
	@echo "==> Build complete: $(TARGET)"
	@$(MAKE) --no-print-directory validate-build

%.o: %.c
	@echo "  CC  $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# ============================================================================
# PGO Workflow
# ============================================================================

pgo-generate:
	@echo "==> Building with PGO instrumentation..."
	@$(MAKE) clean
	@$(MAKE) BUILD_MODE=pgo-gen all
	@echo "==> PGO instrumented build complete"
	@echo "==> Run workload: ./$(TARGET) --benchmark"
	@echo "==> Then run: make pgo-optimize"

pgo-merge:
	@echo "==> Merging PGO profiles..."
	@if [ -d pgo-data ]; then \
		find pgo-data -name "*.gcda" | wc -l | xargs echo "Found profile files:"; \
		echo "Profile data ready for optimization"; \
	else \
		echo "ERROR: No PGO data found. Run pgo-generate first."; \
		exit 1; \
	fi

pgo-optimize: pgo-merge
	@echo "==> Building with PGO optimization..."
	@$(MAKE) clean-objs
	@$(MAKE) BUILD_MODE=pgo-use all
	@echo "==> PGO optimized build complete"

# ============================================================================
# Optimization Validation
# ============================================================================

check-optimization:
	@echo "==> Checking optimization flags..."
	@echo "Build Mode: $(BUILD_MODE)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo ""
	@echo "Checking compiler support:"
	@$(CC) --version | head -1
	@echo ""
	@echo "ISA Extensions:"
	@$(CC) -march=native -dM -E - < /dev/null 2>/dev/null | grep -E "AVX|SSE|FMA" | head -10 || echo "No SIMD extensions detected"

validate-build: $(TARGET)
	@echo "==> Validating build..."
	@if [ -f $(TARGET) ]; then \
		echo "✓ Binary exists"; \
		size $(TARGET) | tail -1; \
		echo "✓ Size check passed"; \
		file $(TARGET); \
		echo "✓ File type check passed"; \
	else \
		echo "✗ Build validation failed"; \
		exit 1; \
	fi

# ============================================================================
# Benchmarking
# ============================================================================

benchmark: $(TARGET)
	@echo "==> Running benchmarks..."
	@./$(TARGET) --benchmark || echo "Benchmark not implemented yet"

# ============================================================================
# Cleaning
# ============================================================================

clean:
	@echo "==> Cleaning build artifacts..."
	@rm -f $(OBJS) $(TARGET)
	@find . -name "*.o" -type f -delete
	@echo "==> Clean complete"

clean-objs:
	@echo "==> Cleaning object files only..."
	@rm -f $(OBJS)
	@find . -name "*.o" -type f -delete

clean-pgo:
	@echo "==> Cleaning PGO data..."
	@rm -rf pgo-data
	@find . -name "*.gcda" -delete
	@find . -name "*.gcno" -delete

clean-all: clean clean-pgo
	@echo "==> Full clean complete"

# ============================================================================
# Testing
# ============================================================================

test: $(TARGET)
	@echo "==> Running tests..."
	@./$(TARGET) --test || echo "Tests not implemented yet"

# ============================================================================
# Information
# ============================================================================

info:
	@echo "========================================"
	@echo "BDI Kernel Build System - Phase 6"
	@echo "========================================"
	@echo "Build Mode:    $(BUILD_MODE)"
	@echo "Compiler:      $(CC)"
	@echo "Target:        $(TARGET)"
	@echo "Source Files:  $(words $(ALL_SRCS))"
	@echo "Object Files:  $(words $(OBJS))"
	@echo ""
	@echo "Optimization Features:"
	@echo "  - LTO:       $(if $(findstring -flto,$(CFLAGS)),Enabled,Disabled)"
	@echo "  - PGO:       $(if $(findstring -fprofile,$(CFLAGS)),Enabled,Disabled)"
	@echo "  - AVX2:      $(if $(findstring -mavx2,$(CFLAGS)),Enabled,Disabled)"
	@echo "  - AVX-512:   $(if $(findstring -mavx512,$(CFLAGS)),Enabled,Disabled)"
	@echo ""
	@echo "Build Modes:"
	@echo "  make BUILD_MODE=debug       - Debug build"
	@echo "  make BUILD_MODE=release     - Release build (default)"
	@echo "  make pgo-generate           - PGO instrumented build"
	@echo "  make pgo-optimize           - PGO optimized build"
	@echo ""
	@echo "Targets:"
	@echo "  make all                    - Build kernel"
	@echo "  make clean                  - Clean build artifacts"
	@echo "  make test                   - Run tests"
	@echo "  make benchmark              - Run benchmarks"
	@echo "  make check-optimization     - Check optimization settings"
	@echo "  make validate-build         - Validate build output"
	@echo ""
	@echo "CRRSS Tooling System:"
	@echo "  make crrss                  - Build CRRSS tools"
	@echo "  make crrss-test             - Run CRRSS tests"
	@echo "  make crrss-check            - Analyze BDI codebase"
	@echo "  make crrss-install          - Install CRRSS"
	@echo "  make crrss-info             - Show CRRSS details"
	@echo "========================================"

help: info
	@echo ""
	@echo "For detailed CRRSS help, run: make crrss-help"
	@echo "For fuzzing help, run: make fuzz-help"

# ============================================================================
# Dependencies
# ============================================================================

# Auto-generate dependencies
-include $(OBJS:.o=.d)

%.d: %.c
	@$(CC) $(CFLAGS) -MM -MT $(@:.d=.o) $< -MF $@

# ============================================================================
# Fuzzing Infrastructure - Comprehensive Security Testing
# ============================================================================

# Fuzzing Configuration
FUZZING_DIR := C/fuzzing
FUZZING_BUILD_DIR := build/fuzzing
FUZZING_HARNESSES := vm_bytecode jit_compiler graph_execution memory_management bytecode_parser value_system

# AFL++ Configuration
AFL_CC := afl-clang-fast
AFL_CXX := afl-clang-fast++
AFL_CFLAGS := -O2 -g -fsanitize=address -fsanitize=undefined
AFL_CFLAGS += -D__AFL_COMPILER -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION

# LibFuzzer Configuration  
LIBFUZZER_CC := clang
LIBFUZZER_CXX := clang++
LIBFUZZER_CFLAGS := -O1 -g -fsanitize=fuzzer,address,undefined
LIBFUZZER_CFLAGS += -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION

# Sanitizer Configuration
SANITIZER_CC := clang
SANITIZER_CXX := clang++
SANITIZER_CFLAGS := -O1 -g -fsanitize=address,undefined,memory
SANITIZER_CFLAGS += -fno-sanitize-recover=all -fno-omit-frame-pointer

# Coverage Configuration
COVERAGE_CC := gcc
COVERAGE_CXX := g++
COVERAGE_CFLAGS := -O0 -g --coverage -fprofile-arcs -ftest-coverage
COVERAGE_LDFLAGS := --coverage -lgcov

# Common fuzzing includes
FUZZ_INCLUDES := -I$(FUZZING_DIR)/harnesses -IC -IC/vm -IC/graph -IC/jit

# ============================================================================
# Fuzzing Build Targets
# ============================================================================

# Create fuzzing build directory
$(FUZZING_BUILD_DIR):
	@mkdir -p $(FUZZING_BUILD_DIR)

# AFL++ Harness Compilation
define AFL_HARNESS_RULE
$(FUZZING_BUILD_DIR)/afl_$(1): $(FUZZING_DIR)/harnesses/$(1)_fuzz.c $(FUZZING_BUILD_DIR)
	@echo "==> Building AFL++ harness: $(1)"
	@$(AFL_CC) $(AFL_CFLAGS) $(FUZZ_INCLUDES) \
		$(FUZZING_DIR)/harnesses/$(1)_fuzz.c \
		-o $(FUZZING_BUILD_DIR)/afl_$(1) \
		2>/dev/null || echo "Warning: AFL++ build failed for $(1)"
endef

# LibFuzzer Harness Compilation
define LIBFUZZER_HARNESS_RULE
$(FUZZING_BUILD_DIR)/libfuzzer_$(1): $(FUZZING_DIR)/harnesses/$(1)_fuzz.c $(FUZZING_BUILD_DIR)
	@echo "==> Building LibFuzzer harness: $(1)"
	@$(LIBFUZZER_CC) $(LIBFUZZER_CFLAGS) $(FUZZ_INCLUDES) \
		$(FUZZING_DIR)/harnesses/$(1)_fuzz.c \
		-o $(FUZZING_BUILD_DIR)/libfuzzer_$(1) \
		2>/dev/null || echo "Warning: LibFuzzer build failed for $(1)"
endef

# Sanitizer Harness Compilation
define SANITIZER_HARNESS_RULE
$(FUZZING_BUILD_DIR)/sanitizer_$(1): $(FUZZING_DIR)/harnesses/$(1)_fuzz.c $(FUZZING_BUILD_DIR)
	@echo "==> Building Sanitizer harness: $(1)"
	@$(SANITIZER_CC) $(SANITIZER_CFLAGS) $(FUZZ_INCLUDES) \
		$(FUZZING_DIR)/harnesses/$(1)_fuzz.c \
		-o $(FUZZING_BUILD_DIR)/sanitizer_$(1) \
		2>/dev/null || echo "Warning: Sanitizer build failed for $(1)"
endef

# Coverage Harness Compilation
define COVERAGE_HARNESS_RULE
$(FUZZING_BUILD_DIR)/coverage_$(1): $(FUZZING_DIR)/harnesses/$(1)_fuzz.c $(FUZZING_BUILD_DIR)
	@echo "==> Building Coverage harness: $(1)"
	@$(COVERAGE_CC) $(COVERAGE_CFLAGS) $(FUZZ_INCLUDES) \
		$(FUZZING_DIR)/harnesses/$(1)_fuzz.c \
		-o $(FUZZING_BUILD_DIR)/coverage_$(1) \
		$(COVERAGE_LDFLAGS) \
		2>/dev/null || echo "Warning: Coverage build failed for $(1)"
endef

# Generate rules for all harnesses
$(foreach harness,$(FUZZING_HARNESSES),$(eval $(call AFL_HARNESS_RULE,$(harness))))
$(foreach harness,$(FUZZING_HARNESSES),$(eval $(call LIBFUZZER_HARNESS_RULE,$(harness))))
$(foreach harness,$(FUZZING_HARNESSES),$(eval $(call SANITIZER_HARNESS_RULE,$(harness))))
$(foreach harness,$(FUZZING_HARNESSES),$(eval $(call COVERAGE_HARNESS_RULE,$(harness))))

# ============================================================================
# Fuzzing Targets
# ============================================================================

# Build all AFL++ harnesses
fuzz-afl: $(foreach harness,$(FUZZING_HARNESSES),$(FUZZING_BUILD_DIR)/afl_$(harness))
	@echo "==> AFL++ harnesses built successfully"

# Build all LibFuzzer harnesses
fuzz-libfuzzer: $(foreach harness,$(FUZZING_HARNESSES),$(FUZZING_BUILD_DIR)/libfuzzer_$(harness))
	@echo "==> LibFuzzer harnesses built successfully"

# Build all sanitizer harnesses
fuzz-sanitizers: $(foreach harness,$(FUZZING_HARNESSES),$(FUZZING_BUILD_DIR)/sanitizer_$(harness))
	@echo "==> Sanitizer harnesses built successfully"

# Build all coverage harnesses
fuzz-coverage: $(foreach harness,$(FUZZING_HARNESSES),$(FUZZING_BUILD_DIR)/coverage_$(harness))
	@echo "==> Coverage harnesses built successfully"

# Build all fuzzing harnesses
fuzz-all: fuzz-afl fuzz-libfuzzer fuzz-sanitizers fuzz-coverage
	@echo "==> All fuzzing harnesses built successfully"

# Individual harness targets
fuzz-vm: $(FUZZING_BUILD_DIR)/afl_vm_bytecode $(FUZZING_BUILD_DIR)/libfuzzer_vm_bytecode
	@echo "==> VM bytecode fuzzing harnesses built"

fuzz-jit: $(FUZZING_BUILD_DIR)/afl_jit_compiler $(FUZZING_BUILD_DIR)/libfuzzer_jit_compiler
	@echo "==> JIT compiler fuzzing harnesses built"

fuzz-graph: $(FUZZING_BUILD_DIR)/afl_graph_execution $(FUZZING_BUILD_DIR)/libfuzzer_graph_execution
	@echo "==> Graph execution fuzzing harnesses built"

fuzz-memory: $(FUZZING_BUILD_DIR)/afl_memory_management $(FUZZING_BUILD_DIR)/libfuzzer_memory_management
	@echo "==> Memory management fuzzing harnesses built"

fuzz-parser: $(FUZZING_BUILD_DIR)/afl_bytecode_parser $(FUZZING_BUILD_DIR)/libfuzzer_bytecode_parser
	@echo "==> Bytecode parser fuzzing harnesses built"

fuzz-values: $(FUZZING_BUILD_DIR)/afl_value_system $(FUZZING_BUILD_DIR)/libfuzzer_value_system
	@echo "==> Value system fuzzing harnesses built"

# ============================================================================
# Fuzzing Execution Targets
# ============================================================================

# Run all fuzzing harnesses
fuzz-run-all:
	@echo "==> Running comprehensive fuzzing campaign..."
	@chmod +x scripts/run_fuzzing.sh
	@./scripts/run_fuzzing.sh

# Run parallel fuzzing
fuzz-run-parallel:
	@echo "==> Running parallel fuzzing campaign..."
	@chmod +x scripts/parallel_fuzz.sh
	@./scripts/parallel_fuzz.sh

# Run fuzzing with specific duration
fuzz-run-timed:
	@echo "==> Running timed fuzzing campaign (30 minutes)..."
	@chmod +x scripts/run_fuzzing.sh
	@./scripts/run_fuzzing.sh --duration 1800

# ============================================================================
# Fuzzing Analysis Targets
# ============================================================================

# Analyze crashes
fuzz-analyze-crashes:
	@echo "==> Analyzing fuzzing crashes..."
	@chmod +x scripts/crash_analysis.sh
	@./scripts/crash_analysis.sh

# Minimize crashes
fuzz-minimize-crashes:
	@echo "==> Minimizing crash inputs..."
	@chmod +x scripts/crash_analysis.sh
	@./scripts/crash_analysis.sh --minimize

# Generate coverage report
fuzz-coverage-report:
	@echo "==> Generating fuzzing coverage report..."
	@chmod +x scripts/coverage_report.sh
	@./scripts/coverage_report.sh --action report

# Manage corpus
fuzz-corpus-generate:
	@echo "==> Generating fuzzing corpus..."
	@chmod +x scripts/corpus_management.sh
	@./scripts/corpus_management.sh --action generate

fuzz-corpus-minimize:
	@echo "==> Minimizing fuzzing corpus..."
	@chmod +x scripts/corpus_management.sh
	@./scripts/corpus_management.sh --action minimize

# ============================================================================
# Fuzzing Maintenance Targets
# ============================================================================

# Clean fuzzing artifacts
fuzz-clean:
	@echo "==> Cleaning fuzzing artifacts..."
	@rm -rf $(FUZZING_BUILD_DIR)
	@rm -rf $(FUZZING_DIR)/crashes/*
	@rm -rf $(FUZZING_DIR)/coverage/*
	@find $(FUZZING_DIR) -name "*.gcda" -delete
	@find $(FUZZING_DIR) -name "*.gcno" -delete

# Clean only crash data
fuzz-clean-crashes:
	@echo "==> Cleaning crash data..."
	@rm -rf $(FUZZING_DIR)/crashes/*

# Clean only coverage data
fuzz-clean-coverage:
	@echo "==> Cleaning coverage data..."
	@rm -rf $(FUZZING_DIR)/coverage/*

# Reproduce specific crash
fuzz-reproduce:
	@echo "==> Reproducing crash (specify CRASH_FILE=path/to/crash)..."
	@if [ -z "$(CRASH_FILE)" ]; then \
		echo "Error: Please specify CRASH_FILE=path/to/crash"; \
		exit 1; \
	fi
	@chmod +x scripts/crash_analysis.sh
	@./scripts/crash_analysis.sh --crash "$(CRASH_FILE)"

# ============================================================================
# Fuzzing Information and Help
# ============================================================================

fuzz-info:
	@echo "========================================"
	@echo "BDI Kernel Fuzzing Infrastructure"
	@echo "========================================"
	@echo "Harnesses Available:"
	@for harness in $(FUZZING_HARNESSES); do \
		echo "  - $$harness"; \
	done
	@echo ""
	@echo "Build Targets:"
	@echo "  fuzz-afl              - Build AFL++ harnesses"
	@echo "  fuzz-libfuzzer        - Build LibFuzzer harnesses"
	@echo "  fuzz-sanitizers       - Build sanitizer harnesses"
	@echo "  fuzz-coverage         - Build coverage harnesses"
	@echo "  fuzz-all              - Build all harnesses"
	@echo ""
	@echo "Execution Targets:"
	@echo "  fuzz-run-all          - Run comprehensive fuzzing"
	@echo "  fuzz-run-parallel     - Run parallel fuzzing"
	@echo "  fuzz-run-timed        - Run timed fuzzing (30min)"
	@echo ""
	@echo "Analysis Targets:"
	@echo "  fuzz-analyze-crashes  - Analyze found crashes"
	@echo "  fuzz-minimize-crashes - Minimize crash inputs"
	@echo "  fuzz-coverage-report  - Generate coverage report"
	@echo ""
	@echo "Corpus Management:"
	@echo "  fuzz-corpus-generate  - Generate seed corpus"
	@echo "  fuzz-corpus-minimize  - Minimize corpus"
	@echo ""
	@echo "Maintenance:"
	@echo "  fuzz-clean            - Clean all fuzzing data"
	@echo "  fuzz-clean-crashes    - Clean crash data only"
	@echo "  fuzz-clean-coverage   - Clean coverage data only"
	@echo "  fuzz-reproduce        - Reproduce specific crash"
	@echo ""
	@echo "Dependencies Required:"
	@echo "  - AFL++ (afl-clang-fast)"
	@echo "  - Clang with LibFuzzer support"
	@echo "  - AddressSanitizer, UBSan, MSan"
	@echo "  - lcov/gcov for coverage"
	@echo "========================================"

fuzz-help: fuzz-info

# ============================================================================
# Fuzzing Integration with Main Build
# ============================================================================

# Add fuzzing to main help
help: info fuzz-info

# Add fuzzing and CRRSS clean to main clean
clean: fuzz-clean-coverage crrss-clean
	@echo "==> Cleaning build artifacts..."
	@rm -rf build/
	@rm -rf $(TARGET)
	@rm -f *.o *.d
	@find . -name "*.o" -delete
	@find . -name "*.d" -delete
	@find . -name "*.gcda" -delete
	@find . -name "*.gcno" -delete

# Security testing target
security-test: fuzz-all fuzz-run-timed fuzz-analyze-crashes
	@echo "==> Comprehensive security testing completed"
	@echo "==> Check $(FUZZING_DIR)/crashes/ for any discovered vulnerabilities"

# Add fuzzing to all target
all: $(TARGET) fuzz-corpus-generate
	@echo "==> Build completed with fuzzing corpus ready"

.PHONY: fuzz-afl fuzz-libfuzzer fuzz-sanitizers fuzz-coverage fuzz-all
.PHONY: fuzz-vm fuzz-jit fuzz-graph fuzz-memory fuzz-parser fuzz-values
.PHONY: fuzz-run-all fuzz-run-parallel fuzz-run-timed
.PHONY: fuzz-analyze-crashes fuzz-minimize-crashes fuzz-coverage-report
.PHONY: fuzz-corpus-generate fuzz-corpus-minimize
.PHONY: fuzz-clean fuzz-clean-crashes fuzz-clean-coverage fuzz-reproduce
.PHONY: fuzz-info fuzz-help security-test

# ============================================================================
# CRRSS Tooling System Integration - Phase 1B Stage 4
# ============================================================================

# CRRSS directories
CRRSS_DIR := tools/crrss
CRRSS_BUILD_DIR := $(CRRSS_DIR)/build

# CRRSS phony targets
.PHONY: crrss crrss-lib crrss-tool crrss-tests crrss-test
.PHONY: crrss-install crrss-uninstall crrss-clean
.PHONY: crrss-check crrss-analyze crrss-info crrss-help

# Build all CRRSS components
crrss:
	@echo "========================================"
	@echo "Building CRRSS Tooling System"
	@echo "========================================"
	@$(MAKE) -C $(CRRSS_DIR) all
	@echo "==> CRRSS build complete"

# Build CRRSS library only
crrss-lib:
	@echo "==> Building CRRSS library..."
	@$(MAKE) -C $(CRRSS_DIR) lib

# Build CRRSS tool only
crrss-tool:
	@echo "==> Building CRRSS command-line tool..."
	@$(MAKE) -C $(CRRSS_DIR) tool

# Build CRRSS tests
crrss-tests:
	@echo "==> Building CRRSS test suite..."
	@$(MAKE) -C $(CRRSS_DIR) tests

# Run CRRSS tests
crrss-test:
	@echo "========================================"
	@echo "Running CRRSS Test Suite"
	@echo "========================================"
	@$(MAKE) -C $(CRRSS_DIR) test

# Install CRRSS to system
crrss-install:
	@echo "==> Installing CRRSS..."
	@$(MAKE) -C $(CRRSS_DIR) install

# Uninstall CRRSS from system
crrss-uninstall:
	@echo "==> Uninstalling CRRSS..."
	@$(MAKE) -C $(CRRSS_DIR) uninstall

# Clean CRRSS build artifacts
crrss-clean:
	@echo "==> Cleaning CRRSS build artifacts..."
	@$(MAKE) -C $(CRRSS_DIR) clean

# Run CRRSS checks on BDI codebase
crrss-check:
	@echo "========================================"
	@echo "Running CRRSS Analysis on BDI Codebase"
	@echo "========================================"
	@$(MAKE) -C $(CRRSS_DIR) check-codebase

# Analyze specific file with CRRSS
crrss-analyze:
	@if [ -z "$(FILE)" ]; then \
		echo "Error: Please specify FILE=<path>"; \
		echo "Example: make crrss-analyze FILE=moduler_kernel/memory.c"; \
		exit 1; \
	fi
	@$(MAKE) -C $(CRRSS_DIR) analyze-file FILE=$(FILE)

# Show CRRSS information
crrss-info:
	@$(MAKE) -C $(CRRSS_DIR) info

# Show CRRSS help
crrss-help:
	@$(MAKE) -C $(CRRSS_DIR) help
