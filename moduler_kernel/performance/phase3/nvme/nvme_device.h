
/**
 * @file nvme_device.h
 * @brief NVMe device discovery and initialization (SPDK-style)
 * 
 * Direct NVMe device access via PCI BAR mapping for ultra-low latency I/O.
 * Implements polling-based completion without kernel involvement.
 */

#ifndef PHASE3_NVME_DEVICE_H
#define PHASE3_NVME_DEVICE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// NVMe register offsets (from NVMe spec)
#define NVME_REG_CAP    0x0000  // Controller Capabilities
#define NVME_REG_VS     0x0008  // Version
#define NVME_REG_CC     0x0014  // Controller Configuration
#define NVME_REG_CSTS   0x001C  // Controller Status
#define NVME_REG_AQA    0x0024  // Admin Queue Attributes
#define NVME_REG_ASQ    0x0028  // Admin Submission Queue Base
#define NVME_REG_ACQ    0x0030  // Admin Completion Queue Base

// Controller Configuration (CC) register bits
#define NVME_CC_ENABLE  (1 << 0)
#define NVME_CC_IOSQES  (6 << 16)  // I/O Submission Queue Entry Size (2^6 = 64 bytes)
#define NVME_CC_IOCQES  (4 << 20)  // I/O Completion Queue Entry Size (2^4 = 16 bytes)

// Controller Status (CSTS) register bits
#define NVME_CSTS_RDY   (1 << 0)
#define NVME_CSTS_CFS   (1 << 1)  // Controller Fatal Status

// Admin command opcodes
#define NVME_ADMIN_CREATE_SQ    0x01
#define NVME_ADMIN_CREATE_CQ    0x05
#define NVME_ADMIN_IDENTIFY     0x06
#define NVME_ADMIN_SET_FEATURES 0x09
#define NVME_ADMIN_GET_FEATURES 0x0A

// NVM command opcodes
#define NVME_CMD_READ   0x02
#define NVME_CMD_WRITE  0x01
#define NVME_CMD_FLUSH  0x00

// Queue sizes
#define NVME_ADMIN_QUEUE_SIZE   64
#define NVME_IO_QUEUE_SIZE      1024
#define NVME_MAX_IO_QUEUES      128

// Forward declarations
typedef struct nvme_device nvme_device_t;
typedef struct nvme_controller_caps nvme_controller_caps_t;
typedef struct nvme_namespace nvme_namespace_t;

/**
 * @brief NVMe controller capabilities
 */
struct nvme_controller_caps {
    uint64_t max_queue_entries;     // Maximum queue entries supported
    uint64_t timeout_ms;            // Timeout in milliseconds
    uint32_t page_size_min;         // Minimum page size
    uint32_t page_size_max;         // Maximum page size
    uint32_t doorbell_stride;       // Doorbell stride (in bytes)
    bool supports_nvm_command_set;  // Supports NVM command set
    bool supports_weighted_rr;      // Supports weighted round robin
};

/**
 * @brief NVMe device structure
 */
struct nvme_device {
    // PCI information
    uint16_t domain;
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    
    // Memory-mapped registers
    volatile void* bar0;            // BAR0 base address
    size_t bar0_size;               // BAR0 size
    
    // Controller capabilities
    nvme_controller_caps_t caps;
    
    // Admin queue pair
    void* admin_sq;                 // Admin submission queue
    void* admin_cq;                 // Admin completion queue
    uint16_t admin_sq_head;
    uint16_t admin_sq_tail;
    uint16_t admin_cq_head;
    uint8_t admin_cq_phase;
    
    // I/O queue pairs (per-core)
    void** io_sq;                   // Array of I/O submission queues
    void** io_cq;                   // Array of I/O completion queues
    uint16_t* io_sq_head;
    uint16_t* io_sq_tail;
    uint16_t* io_cq_head;
    uint8_t* io_cq_phase;
    uint16_t num_io_queues;
    
    // Namespaces
    nvme_namespace_t* namespaces;
    uint32_t num_namespaces;
    
    // Statistics
    uint64_t total_reads;
    uint64_t total_writes;
    uint64_t total_completions;
    uint64_t total_errors;
    
    // NUMA node
    int numa_node;
};

/**
 * @brief Initialize NVMe subsystem
 * 
 * @return 0 on success, negative error code on failure
 */
int nvme_init(void);

/**
 * @brief Shutdown NVMe subsystem
 */
void nvme_shutdown(void);

/**
 * @brief Probe and enumerate NVMe devices
 * 
 * @param devices Output array of discovered devices
 * @param max_devices Maximum number of devices to discover
 * @return Number of devices discovered, negative on error
 */
int nvme_probe_devices(nvme_device_t** devices, size_t max_devices);

/**
 * @brief Attach to an NVMe device
 * 
 * @param domain PCI domain
 * @param bus PCI bus
 * @param device PCI device
 * @param function PCI function
 * @return Device handle on success, NULL on failure
 */
nvme_device_t* nvme_attach_device(uint16_t domain, uint8_t bus, 
                                   uint8_t device, uint8_t function);

/**
 * @brief Detach from an NVMe device
 * 
 * @param dev Device handle
 */
void nvme_detach_device(nvme_device_t* dev);

/**
 * @brief Get controller capabilities
 * 
 * @param dev Device handle
 * @param caps Output capabilities structure
 * @return 0 on success, negative on error
 */
int nvme_get_capabilities(nvme_device_t* dev, nvme_controller_caps_t* caps);

/**
 * @brief Reset NVMe controller
 * 
 * @param dev Device handle
 * @return 0 on success, negative on error
 */
int nvme_reset_controller(nvme_device_t* dev);

/**
 * @brief Enable NVMe controller
 * 
 * @param dev Device handle
 * @return 0 on success, negative on error
 */
int nvme_enable_controller(nvme_device_t* dev);

/**
 * @brief Disable NVMe controller
 * 
 * @param dev Device handle
 * @return 0 on success, negative on error
 */
int nvme_disable_controller(nvme_device_t* dev);

/**
 * @brief Get device statistics
 * 
 * @param dev Device handle
 * @param reads Output total reads
 * @param writes Output total writes
 * @param completions Output total completions
 * @param errors Output total errors
 */
void nvme_get_stats(nvme_device_t* dev, uint64_t* reads, uint64_t* writes,
                    uint64_t* completions, uint64_t* errors);

#ifdef __cplusplus
}
#endif

#endif // PHASE3_NVME_DEVICE_H
