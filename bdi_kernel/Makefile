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
               kernel/memory.c kernel/pmm.c kernel/vmm.c
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
        @echo "BDI Kernel Build Configuration"
        @echo "=============================="
        @echo "Compiler: $(CC)"
        @echo "C Standard: C23"
        @echo "Flags: $(CFLAGS)"
        @echo "Source Files: $(words $(ALL_SRCS))"
        @echo "Target: $(TARGET)"
        @echo ""
        @echo "C23 Features Enabled:"
        @echo "  - nullptr (NULL replacement)"
        @echo "  - [[nodiscard]] attributes"
        @echo "  - constexpr constants"
        @echo "  - _Static_assert"
        @echo ""
        @echo "Note: Full C23 support requires GCC 14+ or Clang 18+"
        @echo "Current compiler: $(shell $(CC) --version | head -1)"

# Dependency tracking
-include $(OBJS:.o=.d)

%.d: %.c
        @$(CC) $(CFLAGS) -MM -MT $(@:.d=.o) $< > $@

