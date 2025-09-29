
/*
 * Master Memory Manager - Phase 4 BDI Integration Implementation
 * Seamless integration with complete BDI kernel
 * Part of the LEGENDARY BDI BUILD
 */

#include "mmm_bdi_integration.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

// Internal BDI integration state
typedef struct {
    mmm_bdi_config_t config;
    bool initialized;
    bool kernel_registered;
    bool memory_connected;
    int kernel_fd;
    void *shared_memory;
    mmm_bdi_interrupt_callback_t interrupt_callback;
    mmm_bdi_memory_callback_t memory_callback;
    mmm_bdi_scheduler_callback_t scheduler_callback;
    pthread_mutex_t integration_mutex;
} mmm_bdi_integration_state_t;

// Global integration state
static mmm_bdi_integration_state_t g_bdi_state = {0};

// Internal function declarations
static int validate_bdi_config(mmm_bdi_config_t *config);
static int setup_shared_memory(void);
static int cleanup_shared_memory(void);
static int connect_to_kernel_interface(void);
static int disconnect_from_kernel_interface(void);

/**
 * Initialize BDI integration
 */
int mmm_bdi_integration_init(mmm_bdi_config_t *config) {
    if (!config) {
        return MMM_ERROR_INVALID_PARAM;
    }
    
    // Validate configuration
    if (validate_bdi_config(config) != MMM_SUCCESS) {
        return MMM_ERROR_INVALID_PARAM;
    }
    
    // CRITICAL FIX: Initialize mutex BEFORE using it
    static bool mutex_initialized = false;
    if (!mutex_initialized) {
        if (pthread_mutex_init(&g_bdi_state.integration_mutex, NULL) != 0) {
            return MMM_ERROR_SYSTEM_FAILURE;
        }
        mutex_initialized = true;
    }
    
    pthread_mutex_lock(&g_bdi_state.integration_mutex);
    
    if (g_bdi_state.initialized) {
        pthread_mutex_unlock(&g_bdi_state.integration_mutex);
        return MMM_SUCCESS;  // Already initialized
    }
    
    // Copy configuration
    memcpy(&g_bdi_state.config, config, sizeof(mmm_bdi_config_t));
    
    // Setup shared memory if required
    if (g_bdi_state.config.shared_memory_size > 0) {
        if (setup_shared_memory() != MMM_SUCCESS) {
            pthread_mutex_unlock(&g_bdi_state.integration_mutex);
            return MMM_ERROR_SYSTEM_FAILURE;
        }
    }
    
    // Connect to kernel interface
    if (connect_to_kernel_interface() != MMM_SUCCESS) {
        cleanup_shared_memory();
        pthread_mutex_unlock(&g_bdi_state.integration_mutex);
        return MMM_ERROR_SYSTEM_FAILURE;
    }
    
    g_bdi_state.initialized = true;
    
    pthread_mutex_unlock(&g_bdi_state.integration_mutex);
    
    return MMM_SUCCESS;
}

/**
 * Register MMM with BDI kernel
 */
int mmm_bdi_kernel_register(mmm_master_control_t *control) {
    if (!control || !g_bdi_state.initialized) {
        return MMM_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&g_bdi_state.integration_mutex);
    
    if (g_bdi_state.kernel_registered) {
        pthread_mutex_unlock(&g_bdi_state.integration_mutex);
        return MMM_SUCCESS;  // Already registered
    }
    
    // Prepare registration data
    struct {
        uint64_t system_id;
        uint32_t mmm_version;
        uint32_t capabilities;
        uint32_t memory_pools;
        uint64_t total_memory;
    } registration_data = {
        .system_id = control->system_id,
        .mmm_version = 0x0400,  // Phase 4
        .capabilities = g_bdi_state.config.callback_flags,
        .memory_pools = g_bdi_state.config.memory_pool_count,
        .total_memory = control->total_memory
    };
    
    // Send registration command to BDI kernel
    struct {
        uint32_t status;
        uint32_t assigned_id;
        uint64_t kernel_timestamp;
    } registration_response;
    
    int result = mmm_bdi_kernel_command(0x1001,  // REGISTER_MMM command
                                       &registration_data, sizeof(registration_data),
                                       &registration_response, sizeof(registration_response));
    
    if (result == MMM_SUCCESS && registration_response.status == 0) {
        g_bdi_state.kernel_registered = true;
    }
    
    pthread_mutex_unlock(&g_bdi_state.integration_mutex);
    
    return result;
}

/**
 * Connect to BDI memory subsystem
 */
int mmm_bdi_memory_subsystem_connect(void) {
    if (!g_bdi_state.initialized) {
        return MMM_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&g_bdi_state.integration_mutex);
    
    if (g_bdi_state.memory_connected) {
        pthread_mutex_unlock(&g_bdi_state.integration_mutex);
        return MMM_SUCCESS;  // Already connected
    }
    
    // Send memory subsystem connection command
    struct {
        uint32_t integration_level;
        uint32_t memory_pool_count;
        uint64_t shared_memory_size;
        uint32_t flags;
    } connect_data = {
        .integration_level = g_bdi_state.config.integration_level,
        .memory_pool_count = g_bdi_state.config.memory_pool_count,
        .shared_memory_size = g_bdi_state.config.shared_memory_size,
        .flags = g_bdi_state.config.callback_flags & MMM_BDI_CALLBACK_MEMORY
    };
    
    struct {
        uint32_t status;
        uint64_t memory_base;
        uint32_t memory_regions;
    } connect_response;
    
    int result = mmm_bdi_kernel_command(0x1002,  // CONNECT_MEMORY command
                                       &connect_data, sizeof(connect_data),
                                       &connect_response, sizeof(connect_response));
    
    if (result == MMM_SUCCESS && connect_response.status == 0) {
        g_bdi_state.memory_connected = true;
    }
    
    pthread_mutex_unlock(&g_bdi_state.integration_mutex);
    
    return result;
}

/**
 * Register interrupt handler with BDI
 */
int mmm_bdi_interrupt_handler_register(mmm_bdi_interrupt_callback_t callback) {
    if (!callback || !g_bdi_state.initialized) {
        return MMM_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&g_bdi_state.integration_mutex);
    
    g_bdi_state.interrupt_callback = callback;
    
    // Register interrupt handler with BDI kernel
    struct {
        uint64_t callback_address;  // In real implementation, this would be a kernel callback
        uint32_t interrupt_mask;
        uint32_t priority;
    } interrupt_data = {
        .callback_address = (uint64_t)(uintptr_t)callback,
        .interrupt_mask = 0xFFFFFFFF,  // All interrupts
        .priority = 1  // High priority
    };
    
    struct {
        uint32_t status;
        uint32_t handler_id;
    } interrupt_response;
    
    int result = mmm_bdi_kernel_command(0x1003,  // REGISTER_INTERRUPT command
                                       &interrupt_data, sizeof(interrupt_data),
                                       &interrupt_response, sizeof(interrupt_response));
    
    pthread_mutex_unlock(&g_bdi_state.integration_mutex);
    
    return result;
}

/**
 * Allocate BDI memory region
 */
int mmm_bdi_allocate_memory_region(uint64_t size, uint32_t flags, 
                                  mmm_bdi_memory_region_t *region) {
    if (!region || size == 0 || !g_bdi_state.initialized) {
        return MMM_ERROR_INVALID_PARAM;
    }
    
    if (!g_bdi_state.memory_connected) {
        return MMM_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&g_bdi_state.integration_mutex);
    
    // Send memory allocation command
    struct {
        uint64_t size;
        uint32_t flags;
        uint32_t alignment;
        uint32_t cache_policy;
    } alloc_data = {
        .size = size,
        .flags = flags,
        .alignment = 4096,  // Page alignment
        .cache_policy = 0   // Default cache policy
    };
    
    struct {
        uint32_t status;
        uint64_t base_address;
        uint64_t actual_size;
        uint32_t region_id;
    } alloc_response;
    
    int result = mmm_bdi_kernel_command(0x2001,  // ALLOCATE_MEMORY command
                                       &alloc_data, sizeof(alloc_data),
                                       &alloc_response, sizeof(alloc_response));
    
    if (result == MMM_SUCCESS && alloc_response.status == 0) {
        // Fill region structure
        memset(region, 0, sizeof(mmm_bdi_memory_region_t));
        region->base_address = alloc_response.base_address;
        region->size = alloc_response.actual_size;
        region->protection_flags = flags;
        region->cache_policy = 0;
        region->shared = (flags & 0x100) != 0;  // Shared flag
        snprintf(region->name, sizeof(region->name), "mmm_region_%u", alloc_response.region_id);
    }
    
    pthread_mutex_unlock(&g_bdi_state.integration_mutex);
    
    return result;
}

/**
 * Map BDI memory region
 */
void* mmm_bdi_map_memory_region(mmm_bdi_memory_region_t *region, void *virtual_address) {
    if (!region || !g_bdi_state.initialized) {
        return NULL;
    }
    
    // In a real implementation, this would use BDI-specific mapping
    // For now, we'll simulate with mmap
    int prot = PROT_READ | PROT_WRITE;
    if (region->protection_flags & 0x01) prot |= PROT_EXEC;
    
    void *mapped = mmap(virtual_address, region->size, prot, 
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (mapped == MAP_FAILED) {
        return NULL;
    }
    
    return mapped;
}

/**
 * Send command to BDI kernel
 */
int mmm_bdi_kernel_command(uint32_t command, void *data, uint32_t data_size,
                          void *response, uint32_t response_size) {
    if (!g_bdi_state.initialized) {
        return MMM_ERROR_NOT_INITIALIZED;
    }
    
    // In a real implementation, this would communicate with the actual BDI kernel
    // For now, we'll simulate successful responses for testing
    
    pthread_mutex_lock(&g_bdi_state.integration_mutex);
    
    // Simulate command processing delay
    usleep(1000);  // 1ms
    
    // Generate simulated responses based on command
    switch (command) {
        case 0x1001:  // REGISTER_MMM
            if (response && response_size >= 16) {
                uint32_t *resp = (uint32_t*)response;
                resp[0] = 0;  // Success status
                resp[1] = 1001;  // Assigned ID
                resp[2] = (uint32_t)time(NULL);  // Timestamp low
                resp[3] = 0;  // Timestamp high
            }
            break;
            
        case 0x1002:  // CONNECT_MEMORY
            if (response && response_size >= 16) {
                uint32_t *resp = (uint32_t*)response;
                resp[0] = 0;  // Success status
                resp[1] = 0x10000000;  // Memory base low
                resp[2] = 0;  // Memory base high
                resp[3] = 64;  // Memory regions
            }
            break;
            
        case 0x1003:  // REGISTER_INTERRUPT
            if (response && response_size >= 8) {
                uint32_t *resp = (uint32_t*)response;
                resp[0] = 0;  // Success status
                resp[1] = 2001;  // Handler ID
            }
            break;
            
        case 0x2001:  // ALLOCATE_MEMORY
            if (response && response_size >= 20) {
                uint32_t *resp = (uint32_t*)response;
                resp[0] = 0;  // Success status
                resp[1] = 0x20000000;  // Base address low
                resp[2] = 0;  // Base address high
                if (data && data_size >= 8) {
                    uint64_t *size_ptr = (uint64_t*)data;
                    resp[3] = (uint32_t)*size_ptr;  // Actual size low
                    resp[4] = (uint32_t)(*size_ptr >> 32);  // Actual size high
                } else {
                    resp[3] = 4096;  // Default size
                    resp[4] = 0;
                }
                // Region ID would be in resp[5] if response is large enough
            }
            break;
            
        default:
            pthread_mutex_unlock(&g_bdi_state.integration_mutex);
            return MMM_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_unlock(&g_bdi_state.integration_mutex);
    
    return MMM_SUCCESS;
}

/**
 * Default interrupt callback
 */
int mmm_bdi_interrupt_callback(mmm_bdi_interrupt_context_t *context) {
    if (!context) {
        return MMM_ERROR_INVALID_PARAM;
    }
    
    // Default interrupt handling - log and acknowledge
    // In a real implementation, this would handle specific interrupt types
    
    return MMM_SUCCESS;
}

/**
 * Cleanup BDI integration
 */
int mmm_bdi_integration_cleanup(void) {
    pthread_mutex_lock(&g_bdi_state.integration_mutex);
    
    if (!g_bdi_state.initialized) {
        pthread_mutex_unlock(&g_bdi_state.integration_mutex);
        return MMM_SUCCESS;
    }
    
    // Unregister from kernel if registered
    if (g_bdi_state.kernel_registered) {
        mmm_bdi_kernel_command(0x1004, NULL, 0, NULL, 0);  // UNREGISTER_MMM
        g_bdi_state.kernel_registered = false;
    }
    
    // Disconnect from memory subsystem
    if (g_bdi_state.memory_connected) {
        mmm_bdi_kernel_command(0x1005, NULL, 0, NULL, 0);  // DISCONNECT_MEMORY
        g_bdi_state.memory_connected = false;
    }
    
    // Cleanup shared memory
    cleanup_shared_memory();
    
    // Disconnect from kernel interface
    disconnect_from_kernel_interface();
    
    // CRITICAL FIX: Unlock and destroy mutex in correct order
    pthread_mutex_unlock(&g_bdi_state.integration_mutex);
    pthread_mutex_destroy(&g_bdi_state.integration_mutex);
    
    // Reset state AFTER mutex cleanup
    memset(&g_bdi_state, 0, sizeof(g_bdi_state));
    
    return MMM_SUCCESS;
}

// Internal helper functions

static int validate_bdi_config(mmm_bdi_config_t *config) {
    if (config->kernel_version < BDI_KERNEL_VERSION_3_0) {
        return MMM_ERROR_INVALID_PARAM;
    }
    
    if (config->integration_level < MMM_BDI_BASIC_INTEGRATION ||
        config->integration_level > MMM_BDI_FULL_INTEGRATION) {
        return MMM_ERROR_INVALID_PARAM;
    }
    
    if (config->shared_memory_size > 0 && config->shared_memory_size < 4096) {
        return MMM_ERROR_INVALID_PARAM;
    }
    
    return MMM_SUCCESS;
}

static int setup_shared_memory(void) {
    if (g_bdi_state.config.shared_memory_size == 0) {
        return MMM_SUCCESS;
    }
    
    // Create shared memory region
    g_bdi_state.shared_memory = mmap(NULL, g_bdi_state.config.shared_memory_size,
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    
    if (g_bdi_state.shared_memory == MAP_FAILED) {
        g_bdi_state.shared_memory = NULL;
        return MMM_ERROR_SYSTEM_FAILURE;
    }
    
    return MMM_SUCCESS;
}

static int cleanup_shared_memory(void) {
    if (g_bdi_state.shared_memory) {
        munmap(g_bdi_state.shared_memory, g_bdi_state.config.shared_memory_size);
        g_bdi_state.shared_memory = NULL;
    }
    
    return MMM_SUCCESS;
}

static int connect_to_kernel_interface(void) {
    // In a real implementation, this would open a device file or socket
    // For simulation, we'll just set a dummy file descriptor
    g_bdi_state.kernel_fd = 1;  // Simulate successful connection
    
    return MMM_SUCCESS;
}

static int disconnect_from_kernel_interface(void) {
    if (g_bdi_state.kernel_fd > 0) {
        // In real implementation: close(g_bdi_state.kernel_fd);
        g_bdi_state.kernel_fd = -1;
    }
    
    return MMM_SUCCESS;
}
