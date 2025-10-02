
/**
 * @file capability.h
 * @brief Capability-based access control for zero-copy IPC
 * 
 * Capabilities provide fine-grained access control for memory regions.
 * Includes spatial bounds (address range) and temporal bounds (validity period).
 */

#ifndef PHASE1_CAPABILITY_H
#define PHASE1_CAPABILITY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Capability permissions (bitmask)
#define CAP_PERM_READ    (1 << 0)
#define CAP_PERM_WRITE   (1 << 1)
#define CAP_PERM_EXECUTE (1 << 2)
#define CAP_PERM_DMA     (1 << 3)

// Trust levels
typedef enum {
    CAP_TRUST_KERNEL = 0,   // Kernel-level trust
    CAP_TRUST_TRUSTED = 1,  // Trusted user-space
    CAP_TRUST_USER = 2      // Untrusted user-space
} cap_trust_level_t;

/**
 * @brief Capability structure
 */
typedef struct {
    uint64_t capability_id;
    uint32_t permissions;       // Bitmask of CAP_PERM_*
    uint32_t trust_level;       // cap_trust_level_t
    
    // Spatial bounds
    void* base_address;
    size_t size;
    
    // Temporal bounds
    uint64_t valid_from;        // Timestamp (nanoseconds)
    uint64_t valid_until;       // Timestamp (nanoseconds)
} capability_t;

/**
 * @brief Create a new capability
 * 
 * @param base_address Base address of memory region
 * @param size Size of memory region
 * @param permissions Permission bitmask
 * @param trust_level Trust level
 * @param valid_from Start time (0 = now)
 * @param valid_until End time (0 = never expires)
 * @return Capability structure
 */
capability_t capability_create(void* base_address,
                                size_t size,
                                uint32_t permissions,
                                cap_trust_level_t trust_level,
                                uint64_t valid_from,
                                uint64_t valid_until);

/**
 * @brief Validate capability
 * 
 * Checks spatial bounds, temporal bounds, and permissions.
 * 
 * @param cap Capability to validate
 * @param ptr Pointer to access
 * @param size Size of access
 * @param required_perms Required permissions
 * @return true if valid, false otherwise
 */
bool capability_validate(const capability_t* cap,
                         void* ptr,
                         size_t size,
                         uint32_t required_perms);

/**
 * @brief Check if capability has permission
 * 
 * @param cap Capability
 * @param perm Permission to check
 * @return true if has permission, false otherwise
 */
bool capability_has_permission(const capability_t* cap, uint32_t perm);

/**
 * @brief Get current timestamp (nanoseconds)
 * 
 * @return Current timestamp
 */
uint64_t capability_get_timestamp(void);

#ifdef __cplusplus
}
#endif

#endif // PHASE1_CAPABILITY_H
