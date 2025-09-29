
/**
 * @file x86_dma_pcie.c
 * @brief x86 DMA & PCIe Queue Management Implementation
 * 
 * Phase 2 Master Memory Manager - Advanced x86 Systems
 * Complete DMA & PCIe queue management for high-performance I/O operations
 */

#include "x86_dma_pcie.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global PCIe and DMA state
static bool pcie_initialized = false;
static pcie_device_t discovered_devices[256];
static uint32_t device_count = 0;
static dma_engine_t dma_engines[4];
static uint32_t dma_engine_count = 0;

// PCIe ECAM (Enhanced Configuration Access Mechanism) base
static volatile uint8_t* pcie_ecam_base = NULL;

// Vendor and class name tables
static const struct {
    uint16_t vendor_id;
    const char* name;
} vendor_names[] = {
    {0x8086, "Intel Corporation"},
    {0x1022, "Advanced Micro Devices"},
    {0x10DE, "NVIDIA Corporation"},
    {0x1002, "ATI Technologies / AMD"},
    {0x15B3, "Mellanox Technologies"},
    {0x14E4, "Broadcom"},
    {0x1137, "Cisco Systems"},
    {0x0000, "Unknown Vendor"}
};

static const struct {
    uint8_t class_code;
    const char* name;
} class_names[] = {
    {0x00, "Unclassified"},
    {0x01, "Mass Storage Controller"},
    {0x02, "Network Controller"},
    {0x03, "Display Controller"},
    {0x04, "Multimedia Controller"},
    {0x05, "Memory Controller"},
    {0x06, "Bridge Device"},
    {0x07, "Communication Controller"},
    {0x08, "Generic System Peripheral"},
    {0x09, "Input Device Controller"},
    {0x0A, "Docking Station"},
    {0x0B, "Processor"},
    {0x0C, "Serial Bus Controller"},
    {0x0D, "Wireless Controller"},
    {0x0E, "Intelligent Controller"},
    {0x0F, "Satellite Communication Controller"},
    {0x10, "Encryption Controller"},
    {0x11, "Signal Processing Controller"},
    {0xFF, "Unknown Class"}
};

/**
 * @brief Calculate PCIe ECAM address
 */
static inline volatile uint8_t* pcie_ecam_addr(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset) {
    if (!pcie_ecam_base) return NULL;
    
    uint32_t address = (bus << 20) | (device << 15) | (function << 12) | offset;
    return pcie_ecam_base + address;
}

/**
 * @brief Initialize PCIe subsystem
 */
int x86_pcie_init(void) {
    if (pcie_initialized) {
        return 0;
    }
    
    // Initialize ECAM base address (typically from ACPI MCFG table)
    // For now, use a common base address
    pcie_ecam_base = (volatile uint8_t*)0xE0000000; // Common ECAM base
    
    // Clear device list
    memset(discovered_devices, 0, sizeof(discovered_devices));
    device_count = 0;
    
    // Scan all buses
    for (uint16_t bus = 0; bus < 256; bus++) {
        x86_pcie_scan_bus(bus);
    }
    
    pcie_initialized = true;
    printf("PCIe initialized: %u devices discovered\n", device_count);
    
    return 0;
}

/**
 * @brief Scan PCIe bus for devices
 */
int x86_pcie_scan_bus(uint8_t bus) {
    for (uint8_t device = 0; device < 32; device++) {
        for (uint8_t function = 0; function < 8; function++) {
            uint16_t vendor_id = x86_pcie_config_read16(bus, device, function, 0);
            
            // Check if device exists
            if (vendor_id == 0xFFFF) {
                if (function == 0) break; // No device, skip other functions
                continue;
            }
            
            // Add device to list
            if (device_count < sizeof(discovered_devices) / sizeof(discovered_devices[0])) {
                pcie_device_t* dev = &discovered_devices[device_count++];
                
                dev->bus = bus;
                dev->device = device;
                dev->function = function;
                
                // Read configuration header
                dev->config.vendor_id = vendor_id;
                dev->config.device_id = x86_pcie_config_read16(bus, device, function, 2);
                dev->config.command = x86_pcie_config_read16(bus, device, function, 4);
                dev->config.status = x86_pcie_config_read16(bus, device, function, 6);
                dev->config.revision_id = x86_pcie_config_read8(bus, device, function, 8);
                dev->config.prog_if = x86_pcie_config_read8(bus, device, function, 9);
                dev->config.subclass = x86_pcie_config_read8(bus, device, function, 10);
                dev->config.class_code = x86_pcie_config_read8(bus, device, function, 11);
                dev->config.header_type = x86_pcie_config_read8(bus, device, function, 14);
                dev->config.capabilities_ptr = x86_pcie_config_read8(bus, device, function, 52);
                
                // Read BARs
                for (int i = 0; i < 6; i++) {
                    dev->config.bar[i] = x86_pcie_config_read32(bus, device, function, 16 + i * 4);
                    
                    // Determine BAR properties
                    if (dev->config.bar[i] != 0) {
                        if (dev->config.bar[i] & 1) {
                            // I/O BAR
                            dev->bars[i].is_memory = false;
                            dev->bars[i].base_address = dev->config.bar[i] & 0xFFFFFFFC;
                        } else {
                            // Memory BAR
                            dev->bars[i].is_memory = true;
                            dev->bars[i].is_64bit = ((dev->config.bar[i] >> 1) & 3) == 2;
                            dev->bars[i].is_prefetchable = (dev->config.bar[i] & 8) != 0;
                            
                            if (dev->bars[i].is_64bit && i < 5) {
                                uint64_t addr = dev->config.bar[i] & 0xFFFFFFF0;
                                addr |= ((uint64_t)dev->config.bar[i + 1]) << 32;
                                dev->bars[i].base_address = addr;
                                i++; // Skip next BAR as it's part of 64-bit address
                            } else {
                                dev->bars[i].base_address = dev->config.bar[i] & 0xFFFFFFF0;
                            }
                        }
                        
                        // Determine BAR size (simplified)
                        dev->bars[i].size = 0x1000; // Default 4KB, should be determined properly
                    }
                }
                
                // Scan capabilities
                dev->capabilities_count = 0;
                uint8_t cap_ptr = dev->config.capabilities_ptr;
                while (cap_ptr && dev->capabilities_count < 16) {
                    uint8_t cap_id = x86_pcie_config_read8(bus, device, function, cap_ptr);
                    uint8_t next_ptr = x86_pcie_config_read8(bus, device, function, cap_ptr + 1);
                    
                    dev->capabilities[dev->capabilities_count].cap_id = cap_id;
                    dev->capabilities[dev->capabilities_count].cap_offset = cap_ptr;
                    dev->capabilities[dev->capabilities_count].cap_data = 
                        x86_pcie_config_read16(bus, device, function, cap_ptr + 2);
                    
                    dev->capabilities_count++;
                    cap_ptr = next_ptr;
                }
                
                printf("PCIe Device: %02x:%02x.%x - %04x:%04x (%s)\n",
                       bus, device, function, vendor_id, dev->config.device_id,
                       x86_pcie_get_class_name(dev->config.class_code));
            }
            
            // If not a multi-function device, skip other functions
            if (function == 0) {
                uint8_t header_type = x86_pcie_config_read8(bus, device, function, 14);
                if (!(header_type & 0x80)) break;
            }
        }
    }
    
    return 0;
}

/**
 * @brief Read 32-bit value from PCIe configuration space
 */
uint32_t x86_pcie_config_read32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    volatile uint32_t* addr = (volatile uint32_t*)pcie_ecam_addr(bus, device, function, offset & 0xFC);
    if (!addr) return 0xFFFFFFFF;
    
    return *addr;
}

/**
 * @brief Read 16-bit value from PCIe configuration space
 */
uint16_t x86_pcie_config_read16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    volatile uint16_t* addr = (volatile uint16_t*)pcie_ecam_addr(bus, device, function, offset & 0xFE);
    if (!addr) return 0xFFFF;
    
    return *addr;
}

/**
 * @brief Read 8-bit value from PCIe configuration space
 */
uint8_t x86_pcie_config_read8(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    volatile uint8_t* addr = pcie_ecam_addr(bus, device, function, offset);
    if (!addr) return 0xFF;
    
    return *addr;
}

/**
 * @brief Write 32-bit value to PCIe configuration space
 */
void x86_pcie_config_write32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    volatile uint32_t* addr = (volatile uint32_t*)pcie_ecam_addr(bus, device, function, offset & 0xFC);
    if (addr) {
        *addr = value;
    }
}

/**
 * @brief Find PCIe device by vendor and device ID
 */
pcie_device_t* x86_pcie_find_device(uint16_t vendor_id, uint16_t device_id) {
    for (uint32_t i = 0; i < device_count; i++) {
        if (discovered_devices[i].config.vendor_id == vendor_id &&
            discovered_devices[i].config.device_id == device_id) {
            return &discovered_devices[i];
        }
    }
    return NULL;
}

/**
 * @brief Find PCIe capability
 */
int x86_pcie_find_capability(pcie_device_t* device, uint8_t cap_id) {
    if (!device) return -1;
    
    for (uint8_t i = 0; i < device->capabilities_count; i++) {
        if (device->capabilities[i].cap_id == cap_id) {
            return device->capabilities[i].cap_offset;
        }
    }
    
    return -1;
}

/**
 * @brief Enable MSI for device
 */
int x86_pcie_enable_msi(pcie_device_t* device, uint16_t vector_count) {
    if (!device) return -1;
    
    int msi_offset = x86_pcie_find_capability(device, PCIE_CAP_ID_MSI);
    if (msi_offset < 0) return -1;
    
    // Read MSI control register
    uint16_t msi_control = x86_pcie_config_read16(device->bus, device->device, 
                                                 device->function, msi_offset + 2);
    
    // Enable MSI
    msi_control |= 0x0001; // MSI Enable bit
    
    // Set multiple message enable
    uint16_t mme = 0;
    if (vector_count > 1) mme = 1;
    if (vector_count > 2) mme = 2;
    if (vector_count > 4) mme = 3;
    if (vector_count > 8) mme = 4;
    if (vector_count > 16) mme = 5;
    if (vector_count > 32) mme = 6;
    
    msi_control = (msi_control & 0xFF8F) | (mme << 4);
    
    x86_pcie_config_write16(device->bus, device->device, device->function, 
                           msi_offset + 2, msi_control);
    
    device->msi_enabled = true;
    device->msi_vector_count = 1 << mme;
    
    return 0;
}

/**
 * @brief Initialize DMA engine
 */
int x86_dma_init_engine(dma_engine_t* engine, uintptr_t mmio_base, uint32_t irq) {
    if (!engine) return -1;
    
    memset(engine, 0, sizeof(*engine));
    
    engine->engine_id = dma_engine_count++;
    engine->mmio_base = mmio_base;
    engine->mmio_size = 0x10000; // 64KB default
    engine->irq_number = irq;
    
    // Initialize channels
    engine->channel_count = DMA_MAX_CHANNELS;
    for (uint32_t i = 0; i < engine->channel_count; i++) {
        dma_channel_t* channel = &engine->channels[i];
        channel->channel_id = i;
        channel->state = DMA_CHANNEL_IDLE;
        
        // Map hardware registers (simplified)
        channel->control_reg = (volatile uint32_t*)(mmio_base + i * 0x100 + 0x00);
        channel->status_reg = (volatile uint32_t*)(mmio_base + i * 0x100 + 0x04);
        channel->src_addr_reg = (volatile uint32_t*)(mmio_base + i * 0x100 + 0x08);
        channel->dst_addr_reg = (volatile uint32_t*)(mmio_base + i * 0x100 + 0x0C);
        channel->count_reg = (volatile uint32_t*)(mmio_base + i * 0x100 + 0x10);
    }
    
    // Set capabilities
    engine->supports_64bit = true;
    engine->supports_scatter_gather = true;
    engine->supports_chaining = true;
    engine->supports_interrupt_coalescing = true;
    engine->max_transfer_size = 0x1000000; // 16MB
    engine->alignment_requirement = DMA_ALIGNMENT_REQUIREMENT;
    
    printf("DMA Engine %u initialized: %u channels, MMIO=0x%lx, IRQ=%u\n",
           engine->engine_id, engine->channel_count, mmio_base, irq);
    
    return 0;
}

/**
 * @brief Allocate DMA channel
 */
dma_channel_t* x86_dma_allocate_channel(dma_engine_t* engine, dma_transfer_type_t type) {
    if (!engine) return NULL;
    
    for (uint32_t i = 0; i < engine->channel_count; i++) {
        dma_channel_t* channel = &engine->channels[i];
        if (channel->state == DMA_CHANNEL_IDLE) {
            channel->state = DMA_CHANNEL_ACTIVE;
            channel->transfer_type = type;
            channel->transfer_mode = DMA_MODE_BLOCK;
            return channel;
        }
    }
    
    return NULL; // No available channels
}

/**
 * @brief Setup DMA transfer
 */
int x86_dma_setup_transfer(dma_channel_t* channel, uintptr_t src, uintptr_t dst, 
                          size_t size, dma_transfer_mode_t mode) {
    if (!channel || channel->state != DMA_CHANNEL_ACTIVE) return -1;
    
    // Check alignment
    if ((src & (DMA_ALIGNMENT_REQUIREMENT - 1)) || 
        (dst & (DMA_ALIGNMENT_REQUIREMENT - 1)) ||
        (size & (DMA_ALIGNMENT_REQUIREMENT - 1))) {
        return -1; // Alignment error
    }
    
    channel->transfer_mode = mode;
    
    // Program hardware registers
    *channel->src_addr_reg = (uint32_t)src;
    *channel->dst_addr_reg = (uint32_t)dst;
    *channel->count_reg = (uint32_t)size;
    
    // Set control register
    uint32_t control = 0;
    control |= (mode & 0x3) << 0;  // Transfer mode
    control |= (channel->transfer_type & 0x3) << 2; // Transfer type
    control |= 1 << 4; // Enable interrupts
    
    *channel->control_reg = control;
    
    return 0;
}

/**
 * @brief Start DMA transfer
 */
int x86_dma_start_transfer(dma_channel_t* channel) {
    if (!channel || channel->state != DMA_CHANNEL_ACTIVE) return -1;
    
    channel->busy = true;
    channel->start_time = __builtin_ia32_rdtsc();
    
    // Start transfer by setting GO bit
    *channel->control_reg |= 1 << 31;
    
    return 0;
}

/**
 * @brief Wait for DMA completion
 */
int x86_dma_wait_completion(dma_channel_t* channel, uint32_t timeout_ms) {
    if (!channel) return -1;
    
    uint64_t start_time = __builtin_ia32_rdtsc();
    uint64_t timeout_cycles = timeout_ms * 3000000ULL; // Approximate cycles for timeout
    
    while (channel->busy) {
        // Check status register
        uint32_t status = *channel->status_reg;
        
        if (status & 1) { // Transfer complete
            channel->busy = false;
            channel->completion_time = __builtin_ia32_rdtsc();
            channel->total_transfers++;
            channel->total_time += channel->completion_time - channel->start_time;
            channel->state = DMA_CHANNEL_IDLE;
            return 0;
        }
        
        if (status & 2) { // Error
            channel->busy = false;
            channel->error_count++;
            channel->state = DMA_CHANNEL_ERROR;
            return -1;
        }
        
        // Check timeout
        if ((__builtin_ia32_rdtsc() - start_time) > timeout_cycles) {
            return -2; // Timeout
        }
        
        // Small delay
        for (int i = 0; i < 100; i++) {
            __asm__ volatile("pause");
        }
    }
    
    return 0;
}

/**
 * @brief Create PCIe queue
 */
int x86_pcie_create_queue(pcie_queue_t* queue, uint32_t queue_size, uint32_t entry_size) {
    if (!queue || queue_size == 0 || entry_size == 0) return -1;
    
    memset(queue, 0, sizeof(*queue));
    
    queue->queue_size = queue_size;
    queue->entry_size = entry_size;
    
    // Allocate queue memory
    queue->entries = malloc(queue_size * sizeof(pcie_queue_entry_t));
    if (!queue->entries) return -1;
    
    memset(queue->entries, 0, queue_size * sizeof(pcie_queue_entry_t));
    
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    
    // Initialize performance metrics
    queue->total_submissions = 0;
    queue->total_completions = 0;
    queue->queue_full_events = 0;
    queue->average_latency_ns = 0;
    
    return 0;
}

/**
 * @brief Submit request to PCIe queue
 */
int x86_pcie_submit_request(pcie_queue_t* queue, pcie_queue_entry_t* entry) {
    if (!queue || !entry) return -1;
    
    // Check if queue is full
    if (queue->count >= queue->queue_size) {
        queue->queue_full_events++;
        return -1;
    }
    
    // Add timestamp
    entry->timestamp = __builtin_ia32_rdtsc();
    
    // Copy entry to queue
    queue->entries[queue->tail] = *entry;
    
    // Update tail pointer
    queue->tail = (queue->tail + 1) % queue->queue_size;
    queue->count++;
    queue->total_submissions++;
    
    // Ring doorbell if configured
    if (queue->tail_doorbell) {
        *queue->tail_doorbell = queue->tail;
    }
    
    return 0;
}

/**
 * @brief Get completion from PCIe queue
 */
int x86_pcie_get_completion(pcie_queue_t* queue, pcie_queue_entry_t* entry) {
    if (!queue || !entry) return -1;
    
    // Check if queue is empty
    if (queue->count == 0) return -1;
    
    // Copy entry from queue
    *entry = queue->entries[queue->head];
    
    // Calculate latency
    uint64_t current_time = __builtin_ia32_rdtsc();
    uint64_t latency_cycles = current_time - entry->timestamp;
    uint64_t latency_ns = latency_cycles / 3; // Approximate conversion to nanoseconds
    
    // Update average latency
    queue->average_latency_ns = (queue->average_latency_ns * queue->total_completions + latency_ns) / 
                               (queue->total_completions + 1);
    
    // Update head pointer
    queue->head = (queue->head + 1) % queue->queue_size;
    queue->count--;
    queue->total_completions++;
    
    // Ring doorbell if configured
    if (queue->head_doorbell) {
        *queue->head_doorbell = queue->head;
    }
    
    return 0;
}

/**
 * @brief Get vendor name
 */
const char* x86_pcie_get_vendor_name(uint16_t vendor_id) {
    for (size_t i = 0; i < sizeof(vendor_names) / sizeof(vendor_names[0]) - 1; i++) {
        if (vendor_names[i].vendor_id == vendor_id) {
            return vendor_names[i].name;
        }
    }
    return vendor_names[sizeof(vendor_names) / sizeof(vendor_names[0]) - 1].name;
}

/**
 * @brief Get class name
 */
const char* x86_pcie_get_class_name(uint8_t class_code) {
    for (size_t i = 0; i < sizeof(class_names) / sizeof(class_names[0]) - 1; i++) {
        if (class_names[i].class_code == class_code) {
            return class_names[i].name;
        }
    }
    return class_names[sizeof(class_names) / sizeof(class_names[0]) - 1].name;
}

/**
 * @brief Get DMA state name
 */
const char* x86_dma_get_state_name(dma_channel_state_t state) {
    switch (state) {
        case DMA_CHANNEL_IDLE: return "IDLE";
        case DMA_CHANNEL_ACTIVE: return "ACTIVE";
        case DMA_CHANNEL_PAUSED: return "PAUSED";
        case DMA_CHANNEL_ERROR: return "ERROR";
        case DMA_CHANNEL_COMPLETED: return "COMPLETED";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Dump PCIe configuration space
 */
void x86_pcie_dump_config_space(pcie_device_t* device) {
    if (!device) return;
    
    printf("PCIe Device %02x:%02x.%x Configuration:\n",
           device->bus, device->device, device->function);
    printf("  Vendor ID: 0x%04x (%s)\n", 
           device->config.vendor_id, x86_pcie_get_vendor_name(device->config.vendor_id));
    printf("  Device ID: 0x%04x\n", device->config.device_id);
    printf("  Class: 0x%02x (%s)\n", 
           device->config.class_code, x86_pcie_get_class_name(device->config.class_code));
    printf("  Subclass: 0x%02x\n", device->config.subclass);
    printf("  Revision: 0x%02x\n", device->config.revision_id);
    printf("  Command: 0x%04x\n", device->config.command);
    printf("  Status: 0x%04x\n", device->config.status);
    printf("  Header Type: 0x%02x\n", device->config.header_type);
    printf("  Capabilities: %u\n", device->capabilities_count);
    
    for (int i = 0; i < 6; i++) {
        if (device->config.bar[i] != 0) {
            printf("  BAR%d: 0x%08x (Base: 0x%lx, Size: %zu, %s)\n",
                   i, device->config.bar[i], device->bars[i].base_address,
                   device->bars[i].size, device->bars[i].is_memory ? "Memory" : "I/O");
        }
    }
}

/**
 * @brief Dump DMA channel state
 */
void x86_dma_dump_channel_state(dma_channel_t* channel) {
    if (!channel) return;
    
    printf("DMA Channel %u State:\n", channel->channel_id);
    printf("  State: %s\n", x86_dma_get_state_name(channel->state));
    printf("  Transfer Type: %u\n", channel->transfer_type);
    printf("  Transfer Mode: %u\n", channel->transfer_mode);
    printf("  Busy: %s\n", channel->busy ? "Yes" : "No");
    printf("  Total Transfers: %lu\n", channel->total_transfers);
    printf("  Total Bytes: %lu\n", channel->total_bytes);
    printf("  Total Time: %lu cycles\n", channel->total_time);
    printf("  Error Count: %u\n", channel->error_count);
    
    if (channel->total_transfers > 0) {
        printf("  Average Transfer Time: %lu cycles\n", 
               channel->total_time / channel->total_transfers);
    }
}
