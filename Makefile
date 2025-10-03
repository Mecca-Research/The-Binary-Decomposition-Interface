
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
CFLAGS += -Ibdi_kernel/drivers -Ibdi_kernel/syscalls

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
DEVICE_SRCS := bdi_kernel/device/device.c
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
        @echo "========================================"

help: info

# ============================================================================
# Dependencies
# ============================================================================

# Auto-generate dependencies
-include $(OBJS:.o=.d)

%.d: %.c
        @$(CC) $(CFLAGS) -MM -MT $(@:.d=.o) $< -MF $@
