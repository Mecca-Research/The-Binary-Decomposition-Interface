
# Device & Hardware Abstraction Layer

## Overview

The Device & Hardware Abstraction Layer provides a comprehensive framework for managing devices in the BDI kernel. It includes device registration, discovery, lifecycle management, hotplug support, interrupt handling, and backend scheduler integration.

## Components

### 1. Device Manager (`device_manager.h/c`)

Core device management system with:
- Device registration and discovery APIs
- Device tree/hierarchy management
- Lifecycle management (probe, attach, detach, remove)
- Reference counting and cleanup
- Device matching and driver binding
- NUMA-aware device placement

**Key Features:**
- Atomic state management with C23 atomics
- Hash-based device lookup for O(1) access
- Type-based device organization
- Automatic driver binding
- Resource management

### 2. Hotplug Subsystem (`hotplug.h/c`)

Unified hotplug event system for:
- Device insertion/removal notifications
- Dynamic device discovery
- State transition management
- Priority-based event handling

**Key Features:**
- Lock-free event queue
- Multiple handler registration
- Device type filtering
- Event statistics tracking

### 3. Interrupt Controller Interface (`irq.h/c`)

Abstraction layer for interrupt controllers with:
- Interrupt registration and handling
- MSI/MSI-X support
- Interrupt routing and affinity management
- Shared and exclusive interrupts

**Key Features:**
- Generic IRQ chip interface
- Per-IRQ handler chains
- Atomic IRQ state management
- Comprehensive statistics

### 4. Standard Driver Interface (`driver_interface.h/c`)

Standard file operations and driver API:
- File operations (open, close, read, write, ioctl, mmap)
- Driver registration
- Device node creation
- Blocking and non-blocking I/O
- DMA helper functions

**Key Features:**
- POSIX-like file operations
- Vectored I/O support
- DMA buffer management
- Memory mapping support

### 5. Device Classes (`device_class.h/c`)

Device class framework for:
- Block devices (storage)
- Character devices (serial, terminals)
- Network devices (NICs)
- Input devices (keyboard, mouse)
- Display devices (framebuffer, GPU)
- Timer devices (RTC, HPET)
- Power devices (ACPI, battery)

**Key Features:**
- Class-specific operations
- Device enumeration
- Automatic class assignment

### 6. Backend Integration (`backend_integration.h/c`)

Backend scheduler integration for:
- Device I/O routing to backends (CPU/GPU/FPGA/BPU)
- Device affinity management
- NUMA-aware placement
- Accelerator-backed I/O

**Key Features:**
- Automatic backend routing
- Per-device backend affinity
- I/O request queuing
- Backend statistics

## Usage Examples

### Registering a Device

```c
struct device *dev = malloc(sizeof(struct device));
memset(dev, 0, sizeof(struct device));

snprintf(dev->name, DEVICE_NAME_MAX, "mydevice0");
snprintf(dev->path, DEVICE_PATH_MAX, "/dev/mydevice0");
dev->type = DEVICE_TYPE_BLOCK;
dev->ops = &my_device_ops;

int result = device_register(dev);
```

### Registering a Driver

```c
struct device_driver driver = {
    .name = "mydriver",
    .type = DEVICE_TYPE_BLOCK,
    .ops = &my_driver_ops,
    .match = my_driver_match,
    .bind = my_driver_bind,
    .unbind = my_driver_unbind
};

device_driver_register(&driver);
```

### Handling Interrupts

```c
int my_irq_handler(uint32_t irq, void *dev_id) {
    // Handle interrupt
    return IRQ_HANDLED;
}

irq_request(10, my_irq_handler, IRQ_FLAG_SHARED, "mydevice", dev);
```

### Submitting I/O Requests

```c
struct io_request req = {
    .device = dev,
    .op_type = IO_OP_READ,
    .buffer = buffer,
    .size = 4096,
    .offset = 0,
    .priority = IO_PRIORITY_NORMAL,
    .completion = my_completion_callback
};

backend_submit_io(&req);
```

## Integration with Existing Kernel

The device abstraction layer integrates with:
- **Scheduler**: Device I/O operations are routed through backend schedulers
- **Memory Management**: DMA buffers use VMM for physical address translation
- **Process Management**: Device files can be opened by processes
- **Backend Dispatch**: Device work is distributed to appropriate backends

## Testing

Run the test suite:

```bash
make test_device_manager
./test_device_manager
```

The test suite covers:
- Device manager initialization
- Device registration and discovery
- Hotplug events
- IRQ handling
- Device classes
- Backend integration
- DMA operations

## Statistics and Monitoring

Get device statistics:

```c
uint64_t total, active;
device_manager_get_stats(&total, &active);
printf("Total devices: %lu, Active: %lu\n", total, active);
```

Get IRQ statistics:

```c
uint64_t total_irqs, spurious, unhandled;
irq_get_global_stats(&total_irqs, &spurious, &unhandled);
```

Get backend statistics:

```c
uint64_t total, completed, failed, pending;
backend_get_stats(BACKEND_TYPE_CPU, &total, &completed, &failed, &pending);
```

## Future Enhancements

- Power management (suspend/resume)
- Device firmware loading
- Hot-swap support for PCIe devices
- Advanced DMA scatter-gather
- Device tree parsing
- ACPI integration
- USB device support
- Network device queuing

## License

Part of the BDI kernel project.
