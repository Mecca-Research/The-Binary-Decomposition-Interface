
/*
 * Master Memory Manager - Phase 4 BDI Integration
 * Seamless integration with complete BDI kernel
 * Part of the LEGENDARY BDI BUILD
 */

#ifndef MMM_BDI_INTEGRATION_H
#define MMM_BDI_INTEGRATION_H

#include <stdint.h>
#include <stdbool.h>
#include "../orchestrator/mmm_orchestrator.h"

// BDI kernel version compatibility
#define BDI_KERNEL_VERSION_4_0  0x0400
#define BDI_KERNEL_VERSION_3_0  0x0300

// Integration levels
typedef enum {
    MMM_BDI_BASIC_INTEGRATION = 1,
    MMM_BDI_ADVANCED_INTEGRATION,
    MMM_BDI_FULL_INTEGRATION
} mmm_bdi_integration_level_t;

// Callback flags
#define MMM_BDI_CALLBACK_MEMORY     0x01
#define MMM_BDI_CALLBACK_INTERRUPT  0x02
#define MMM_BDI_CALLBACK_SCHEDULER  0x04
#define MMM_BDI_CALLBACK_SECURITY   0x08
#define MMM_BDI_CALLBACK_ALL        0xFF

// BDI configuration
typedef struct {
    uint32_t kernel_version;
    mmm_bdi_integration_level_t integration_level;
    uint32_t callback_flags;
    uint32_t memory_pool_count;
    uint64_t shared_memory_size;
    bool real_time_enabled;
    bool security_enabled;
    char kernel_interface_path[256];
} mmm_bdi_config_t;

// BDI memory subsystem interface
typedef struct {
    uint64_t base_address;
    uint64_t size;
    uint32_t protection_flags;
    uint32_t cache_policy;
    bool shared;
    char name[64];
} mmm_bdi_memory_region_t;

// BDI interrupt context
typedef struct {
    uint32_t interrupt_number;
    uint32_t priority;
    uint64_t timestamp;
    void *context_data;
    uint32_t data_size;
} mmm_bdi_interrupt_context_t;

// BDI scheduler interface
typedef struct {
    uint32_t task_id;
    uint32_t priority;
    uint32_t cpu_affinity;
    uint64_t memory_quota;
    uint32_t time_slice_ms;
    bool real_time;
} mmm_bdi_task_info_t;

// Callback function types
typedef int (*mmm_bdi_memory_callback_t)(mmm_bdi_memory_region_t *region, uint32_t operation);
typedef int (*mmm_bdi_interrupt_callback_t)(mmm_bdi_interrupt_context_t *context);
typedef int (*mmm_bdi_scheduler_callback_t)(mmm_bdi_task_info_t *task, uint32_t event);

// Function declarations

/**
 * Initialize BDI integration
 * @param config BDI configuration
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_bdi_integration_init(mmm_bdi_config_t *config);

/**
 * Register MMM with BDI kernel
 * @param control Master control structure
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_bdi_kernel_register(mmm_master_control_t *control);

/**
 * Unregister MMM from BDI kernel
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_bdi_kernel_unregister(void);

/**
 * Connect to BDI memory subsystem
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_bdi_memory_subsystem_connect(void);

/**
 * Disconnect from BDI memory subsystem
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_bdi_memory_subsystem_disconnect(void);

/**
 * Register interrupt handler with BDI
 * @param callback Interrupt callback function
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_bdi_interrupt_handler_register(mmm_bdi_interrupt_callback_t callback);

/**
 * Unregister interrupt handler from BDI
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_bdi_interrupt_handler_unregister(void);

/**
 * Register scheduler callback with BDI
 * @param callback Scheduler callback function
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_bdi_scheduler_callback_register(mmm_bdi_scheduler_callback_t callback);

/**
 * Allocate BDI memory region
 * @param size Size of memory region
 * @param flags Allocation flags
 * @param region Output memory region information
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_bdi_allocate_memory_region(uint64_t size, uint32_t flags, 
                                  mmm_bdi_memory_region_t *region);

/**
 * Free BDI memory region
 * @param region Memory region to free
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_bdi_free_memory_region(mmm_bdi_memory_region_t *region);

/**
 * Map BDI memory region
 * @param region Memory region to map
 * @param virtual_address Desired virtual address (0 for automatic)
 * @return Mapped virtual address on success, NULL on failure
 */
void* mmm_bdi_map_memory_region(mmm_bdi_memory_region_t *region, void *virtual_address);

/**
 * Unmap BDI memory region
 * @param virtual_address Virtual address to unmap
 * @param size Size of region to unmap
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_bdi_unmap_memory_region(void *virtual_address, uint64_t size);

/**
 * Set memory protection for BDI region
 * @param region Memory region
 * @param protection_flags Protection flags
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_bdi_set_memory_protection(mmm_bdi_memory_region_t *region, uint32_t protection_flags);

/**
 * Get BDI kernel statistics
 * @param stats Output statistics structure
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_bdi_get_kernel_stats(void *stats);

/**
 * Synchronize with BDI kernel
 * @param timeout_ms Timeout in milliseconds
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_bdi_kernel_sync(uint32_t timeout_ms);

/**
 * Send command to BDI kernel
 * @param command Command code
 * @param data Command data
 * @param data_size Size of command data
 * @param response Response buffer
 * @param response_size Size of response buffer
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_bdi_kernel_command(uint32_t command, void *data, uint32_t data_size,
                          void *response, uint32_t response_size);

/**
 * Get BDI integration status
 * @param status Output status structure
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_bdi_get_integration_status(void *status);

/**
 * Cleanup BDI integration
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_bdi_integration_cleanup(void);

// Default interrupt callback (can be overridden)
int mmm_bdi_interrupt_callback(mmm_bdi_interrupt_context_t *context);

// Default memory callback (can be overridden)
int mmm_bdi_memory_callback(mmm_bdi_memory_region_t *region, uint32_t operation);

// Default scheduler callback (can be overridden)
int mmm_bdi_scheduler_callback(mmm_bdi_task_info_t *task, uint32_t event);

#endif /* MMM_BDI_INTEGRATION_H */
