
# BDI Kernel Drivers

This directory contains device drivers for the Binary Decomposition Interface (BDI) Kernel.

## Driver Categories

### Block Devices
- **block_device.c** - Generic block device interface
- **ramdisk.c** - RAM disk driver for testing and temporary storage

### Character Devices  
- **char_device.c** - Generic character device interface
- **null_device.c** - Null device driver (/dev/null equivalent)
- **zero_device.c** - Zero device driver (/dev/zero equivalent)
- **random_device.c** - Random number generator device

### Network Devices
- **network_device.c** - Generic network device interface
- **loopback.c** - Loopback network interface
- **ethernet.c** - Basic Ethernet driver framework

### Input Devices
- **input_device.c** - Generic input device interface
- **keyboard.c** - PS/2 keyboard driver
- **mouse.c** - PS/2 mouse driver

### Display Devices
- **framebuffer.c** - Generic framebuffer interface
- **vga_text.c** - VGA text mode driver
- **vesa.c** - VESA graphics driver

### Timer Devices
- **timer.c** - System timer interface
- **rtc.c** - Real-time clock driver

### Power Management
- **power.c** - Power management interface
- **acpi.c** - ACPI power management

## Driver Architecture

All BDI drivers follow a common architecture:

1. **Device Registration** - Drivers register with the kernel device manager
2. **Standard Interface** - Common operations (open, close, read, write, ioctl)
3. **Interrupt Handling** - Standardized interrupt management
4. **Resource Management** - Memory and I/O resource allocation
5. **Error Handling** - Consistent error reporting and recovery

## Building Drivers

Drivers are compiled as part of the BDI kernel build process. Each driver includes:

- Header file with device interface definitions
- Implementation file with driver logic
- Makefile integration for kernel build
- Documentation and usage examples

## Adding New Drivers

To add a new driver:

1. Create driver source file in appropriate subdirectory
2. Implement standard device interface functions
3. Add driver to kernel build system
4. Update device manager registration
5. Add documentation and tests

## License

All drivers are released under the same license as the BDI Kernel.
