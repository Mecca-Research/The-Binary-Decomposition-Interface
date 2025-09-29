
/**
 * @file x86_dma_pcie.h
 * @brief x86 DMA & PCIe Queue Management for High-Performance I/O
 * 
 * Phase 2 Master Memory Manager - Advanced x86 Systems
 * Complete DMA & PCIe queue management for high-performance I/O operations
 */

#ifndef X86_DMA_PCIE_H
#define X86_DMA_PCIE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// PCIe Configuration Constants
#define PCIE_MAX_FUNCTIONS          8
#define PCIE_MAX_DEVICES           32
#define PCIE_MAX_BUSES            256
#define PCIE_CONFIG_SPACE_SIZE    256
#define PCIE_EXTENDED_CONFIG_SIZE 4096

// DMA Constants
#define DMA_MAX_CHANNELS           16
#define DMA_MAX_DESCRIPTORS      1024
#define DMA_MAX_SCATTER_GATHER     64
#define DMA_ALIGNMENT_REQUIREMENT  64

// PCIe Capability IDs
#define PCIE_CAP_ID_PM             0x01
#define PCIE_CAP_ID_MSI            0x05
#define PCIE_CAP_ID_VENDOR         0x09
#define PCIE_CAP_ID_MSIX           0x11
#define PCIE_CAP_ID_PCIE           0x10

// PCIe Extended Capability IDs
#define PCIE_EXT_CAP_ID_AER        0x0001
#define PCIE_EXT_CAP_ID_VC         0x0002
#define PCIE_EXT_CAP_ID_DSN        0x0003
#define PCIE_EXT_CAP_ID_PWR        0x0004
#define PCIE_EXT_CAP_ID_SRIOV      0x0010

// DMA Transfer Types
typedef enum {
    DMA_TRANSFER_MEMORY_TO_MEMORY = 0,
    DMA_TRANSFER_MEMORY_TO_DEVICE,
    DMA_TRANSFER_DEVICE_TO_MEMORY,
    DMA_TRANSFER_DEVICE_TO_DEVICE
} dma_transfer_type_t;

// DMA Transfer Modes
typedef enum {
    DMA_MODE_SINGLE = 0,
    DMA_MODE_BLOCK,
    DMA_MODE_DEMAND,
    DMA_MODE_CASCADE
} dma_transfer_mode_t;

// DMA Channel States
typedef enum {
    DMA_CHANNEL_IDLE = 0,
    DMA_CHANNEL_ACTIVE,
    DMA_CHANNEL_PAUSED,
    DMA_CHANNEL_ERROR,
    DMA_CHANNEL_COMPLETED
} dma_channel_state_t;

// PCIe Link Speeds
typedef enum {
    PCIE_LINK_SPEED_2_5GT = 1,
    PCIE_LINK_SPEED_5_0GT = 2,
    PCIE_LINK_SPEED_8_0GT = 3,
    PCIE_LINK_SPEED_16_0GT = 4,
    PCIE_LINK_SPEED_32_0GT = 5
} pcie_link_speed_t;

// PCIe Link Widths
typedef enum {
    PCIE_LINK_WIDTH_X1 = 1,
    PCIE_LINK_WIDTH_X2 = 2,
    PCIE_LINK_WIDTH_X4 = 4,
    PCIE_LINK_WIDTH_X8 = 8,
    PCIE_LINK_WIDTH_X16 = 16,
    PCIE_LINK_WIDTH_X32 = 32
} pcie_link_width_t;

/**
 * @brief PCIe Configuration Space Header
 */
typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t revision_id;
    uint8_t prog_if;
    uint8_t subclass;
    uint8_t class_code;
    uint8_t cache_line_size;
    uint8_t latency_timer;
    uint8_t header_type;
    uint8_t bist;
    uint32_t bar[6];
    uint32_t cardbus_cis_ptr;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;
    uint32_t expansion_rom_base;
    uint8_t capabilities_ptr;
    uint8_t reserved[7];
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint8_t min_gnt;
    uint8_t max_lat;
} __attribute__((packed)) pcie_config_header_t;

/**
 * @brief PCIe Device Structure
 */
typedef struct {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    pcie_config_header_t config;
    
    // Capability information
    uint8_t capabilities_count;
    struct {
        uint8_t cap_id;
        uint8_t cap_offset;
        uint16_t cap_data;
    } capabilities[16];
    
    // BAR information
    struct {
        uintptr_t base_address;
        size_t size;
        bool is_memory;
        bool is_64bit;
        bool is_prefetchable;
    } bars[6];
    
    // MSI/MSI-X information
    bool msi_enabled;
    bool msix_enabled;
    uint16_t msi_vector_count;
    uint16_t msix_vector_count;
    
    // Link information
    pcie_link_speed_t link_speed;
    pcie_link_width_t link_width;
    
    // Device-specific data
    void* device_data;
    
} pcie_device_t;

/**
 * @brief DMA Scatter-Gather Entry
 */
typedef struct {
    uintptr_t address;      // Physical address
    uint32_t length;        // Transfer length
    uint32_t flags;         // Control flags
} dma_sg_entry_t;

/**
 * @brief DMA Descriptor
 */
typedef struct {
    uint32_t control;           // Control flags
    uint32_t status;            // Status flags
    uintptr_t src_address;      // Source address
    uintptr_t dst_address;      // Destination address
    uint32_t transfer_size;     // Transfer size in bytes
    uint32_t next_descriptor;   // Next descriptor (for chaining)
    
    // Scatter-gather list
    uint32_t sg_count;
    dma_sg_entry_t sg_list[DMA_MAX_SCATTER_GATHER];
    
    // Completion callback
    void (*completion_callback)(void* context, int status);
    void* callback_context;
    
    // Timing information
    uint64_t start_time;
    uint64_t completion_time;
    
} dma_descriptor_t;

/**
 * @brief DMA Channel Structure
 */
typedef struct {
    uint8_t channel_id;
    dma_channel_state_t state;
    dma_transfer_type_t transfer_type;
    dma_transfer_mode_t transfer_mode;
    
    // Hardware registers (memory-mapped)
    volatile uint32_t* control_reg;
    volatile uint32_t* status_reg;
    volatile uint32_t* src_addr_reg;
    volatile uint32_t* dst_addr_reg;
    volatile uint32_t* count_reg;
    
    // Descriptor queue
    dma_descriptor_t* descriptor_ring;
    uint32_t descriptor_count;
    uint32_t head_index;
    uint32_t tail_index;
    
    // Performance metrics
    uint64_t total_transfers;
    uint64_t total_bytes;
    uint64_t total_time;
    uint32_t error_count;
    
    // Synchronization
    volatile bool busy;
    void* completion_event;
    
} dma_channel_t;

/**
 * @brief DMA Engine Structure
 */
typedef struct {
    uint32_t engine_id;
    uint32_t channel_count;
    dma_channel_t channels[DMA_MAX_CHANNELS];
    
    // Hardware information
    uintptr_t mmio_base;
    size_t mmio_size;
    uint32_t irq_number;
    
    // Capabilities
    bool supports_64bit;
    bool supports_scatter_gather;
    bool supports_chaining;
    bool supports_interrupt_coalescing;
    uint32_t max_transfer_size;
    uint32_t alignment_requirement;
    
    // Performance monitoring
    uint64_t total_interrupts;
    uint64_t spurious_interrupts;
    
} dma_engine_t;

/**
 * @brief PCIe Queue Entry
 */
typedef struct {
    uint64_t address;       // Address for the operation
    uint32_t length;        // Length of the operation
    uint32_t flags;         // Operation flags
    uint16_t tag;           // Transaction tag
    uint16_t requester_id;  // Requester ID
    uint64_t timestamp;     // Timestamp
} pcie_queue_entry_t;

/**
 * @brief PCIe Queue Structure
 */
typedef struct {
    uint32_t queue_id;
    uint32_t queue_size;
    uint32_t entry_size;
    
    // Queue memory
    pcie_queue_entry_t* entries;
    volatile uint32_t head;
    volatile uint32_t tail;
    volatile uint32_t count;
    
    // Hardware doorbell registers
    volatile uint32_t* head_doorbell;
    volatile uint32_t* tail_doorbell;
    
    // Interrupt configuration
    uint16_t interrupt_vector;
    bool interrupt_enabled;
    uint32_t interrupt_coalescing_count;
    uint32_t interrupt_coalescing_time;
    
    // Performance metrics
    uint64_t total_submissions;
    uint64_t total_completions;
    uint64_t queue_full_events;
    uint64_t average_latency_ns;
    
} pcie_queue_t;

// Core PCIe Functions
int x86_pcie_init(void);
int x86_pcie_scan_bus(uint8_t bus);
pcie_device_t* x86_pcie_find_device(uint16_t vendor_id, uint16_t device_id);
int x86_pcie_enumerate_devices(pcie_device_t* devices, uint32_t max_devices);

// PCIe Configuration Space Access
uint32_t x86_pcie_config_read32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
uint16_t x86_pcie_config_read16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
uint8_t x86_pcie_config_read8(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void x86_pcie_config_write32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);
void x86_pcie_config_write16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t value);
void x86_pcie_config_write8(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint8_t value);

// PCIe Capability Management
int x86_pcie_find_capability(pcie_device_t* device, uint8_t cap_id);
int x86_pcie_find_extended_capability(pcie_device_t* device, uint16_t cap_id);
int x86_pcie_enable_msi(pcie_device_t* device, uint16_t vector_count);
int x86_pcie_enable_msix(pcie_device_t* device, uint16_t vector_count);
int x86_pcie_disable_msi(pcie_device_t* device);
int x86_pcie_disable_msix(pcie_device_t* device);

// PCIe BAR Management
int x86_pcie_map_bar(pcie_device_t* device, uint8_t bar_index, void** mapped_address);
int x86_pcie_unmap_bar(pcie_device_t* device, uint8_t bar_index);
size_t x86_pcie_get_bar_size(pcie_device_t* device, uint8_t bar_index);
bool x86_pcie_is_bar_memory(pcie_device_t* device, uint8_t bar_index);

// DMA Engine Management
int x86_dma_init_engine(dma_engine_t* engine, uintptr_t mmio_base, uint32_t irq);
int x86_dma_shutdown_engine(dma_engine_t* engine);
dma_channel_t* x86_dma_allocate_channel(dma_engine_t* engine, dma_transfer_type_t type);
int x86_dma_free_channel(dma_channel_t* channel);

// DMA Transfer Operations
int x86_dma_setup_transfer(dma_channel_t* channel, uintptr_t src, uintptr_t dst, 
                          size_t size, dma_transfer_mode_t mode);
int x86_dma_setup_scatter_gather(dma_channel_t* channel, dma_sg_entry_t* sg_list, 
                                uint32_t sg_count);
int x86_dma_start_transfer(dma_channel_t* channel);
int x86_dma_pause_transfer(dma_channel_t* channel);
int x86_dma_resume_transfer(dma_channel_t* channel);
int x86_dma_abort_transfer(dma_channel_t* channel);
int x86_dma_wait_completion(dma_channel_t* channel, uint32_t timeout_ms);

// DMA Descriptor Management
int x86_dma_create_descriptor_ring(dma_channel_t* channel, uint32_t descriptor_count);
int x86_dma_destroy_descriptor_ring(dma_channel_t* channel);
int x86_dma_submit_descriptor(dma_channel_t* channel, dma_descriptor_t* descriptor);
dma_descriptor_t* x86_dma_get_completed_descriptor(dma_channel_t* channel);

// PCIe Queue Management
int x86_pcie_create_queue(pcie_queue_t* queue, uint32_t queue_size, uint32_t entry_size);
int x86_pcie_destroy_queue(pcie_queue_t* queue);
int x86_pcie_submit_request(pcie_queue_t* queue, pcie_queue_entry_t* entry);
int x86_pcie_get_completion(pcie_queue_t* queue, pcie_queue_entry_t* entry);
bool x86_pcie_queue_is_full(const pcie_queue_t* queue);
bool x86_pcie_queue_is_empty(const pcie_queue_t* queue);
uint32_t x86_pcie_queue_get_count(const pcie_queue_t* queue);

// High-Performance I/O Operations
int x86_pcie_bulk_read(pcie_device_t* device, uintptr_t device_addr, void* host_buffer, 
                      size_t size, uint32_t queue_id);
int x86_pcie_bulk_write(pcie_device_t* device, uintptr_t device_addr, const void* host_buffer, 
                       size_t size, uint32_t queue_id);
int x86_pcie_async_read(pcie_device_t* device, uintptr_t device_addr, void* host_buffer, 
                       size_t size, void (*callback)(void*, int), void* context);
int x86_pcie_async_write(pcie_device_t* device, uintptr_t device_addr, const void* host_buffer, 
                        size_t size, void (*callback)(void*, int), void* context);

// Interrupt Management
int x86_pcie_register_interrupt_handler(pcie_device_t* device, uint16_t vector, 
                                       void (*handler)(void*), void* context);
int x86_pcie_unregister_interrupt_handler(pcie_device_t* device, uint16_t vector);
int x86_pcie_enable_interrupts(pcie_device_t* device);
int x86_pcie_disable_interrupts(pcie_device_t* device);

// Performance Monitoring
typedef struct {
    uint64_t total_transfers;
    uint64_t total_bytes;
    uint64_t total_time_ns;
    uint64_t average_bandwidth_mbps;
    uint64_t peak_bandwidth_mbps;
    uint32_t error_count;
    uint32_t timeout_count;
    uint32_t retry_count;
} pcie_perf_stats_t;

int x86_pcie_get_performance_stats(pcie_device_t* device, pcie_perf_stats_t* stats);
int x86_pcie_reset_performance_stats(pcie_device_t* device);
void x86_pcie_print_performance_stats(const pcie_perf_stats_t* stats);

// Power Management
int x86_pcie_set_power_state(pcie_device_t* device, uint8_t power_state);
uint8_t x86_pcie_get_power_state(pcie_device_t* device);
int x86_pcie_enable_aspm(pcie_device_t* device, uint8_t aspm_level);
int x86_pcie_disable_aspm(pcie_device_t* device);

// Error Handling and Recovery
typedef enum {
    PCIE_ERROR_CORRECTABLE = 0,
    PCIE_ERROR_UNCORRECTABLE_NON_FATAL,
    PCIE_ERROR_UNCORRECTABLE_FATAL
} pcie_error_type_t;

typedef struct {
    pcie_error_type_t type;
    uint32_t error_code;
    uint64_t timestamp;
    const char* description;
} pcie_error_info_t;

int x86_pcie_enable_aer(pcie_device_t* device);
int x86_pcie_disable_aer(pcie_device_t* device);
int x86_pcie_get_error_info(pcie_device_t* device, pcie_error_info_t* error_info);
int x86_pcie_clear_errors(pcie_device_t* device);
int x86_pcie_reset_device(pcie_device_t* device);

// SR-IOV Support
int x86_pcie_enable_sriov(pcie_device_t* device, uint16_t num_vfs);
int x86_pcie_disable_sriov(pcie_device_t* device);
int x86_pcie_get_vf_device(pcie_device_t* pf_device, uint16_t vf_index, pcie_device_t* vf_device);

// Debugging and Diagnostics
void x86_pcie_dump_config_space(pcie_device_t* device);
void x86_pcie_dump_capabilities(pcie_device_t* device);
void x86_pcie_dump_bars(pcie_device_t* device);
void x86_dma_dump_channel_state(dma_channel_t* channel);
void x86_dma_dump_descriptor_ring(dma_channel_t* channel);
void x86_pcie_dump_queue_state(pcie_queue_t* queue);

// Utility Functions
const char* x86_pcie_get_class_name(uint8_t class_code);
const char* x86_pcie_get_vendor_name(uint16_t vendor_id);
const char* x86_dma_get_state_name(dma_channel_state_t state);
const char* x86_pcie_get_link_speed_name(pcie_link_speed_t speed);
const char* x86_pcie_get_link_width_name(pcie_link_width_t width);

#ifdef __cplusplus
}
#endif

#endif // X86_DMA_PCIE_H
