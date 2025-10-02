
/**
 * @file descriptor.h
 * @brief Memory descriptor for zero-copy IPC
 * 
 * Descriptors encapsulate (ptr, len, capability) tuples for passing
 * memory references without copying data.
 */

#ifndef PHASE1_DESCRIPTOR_H
#define PHASE1_DESCRIPTOR_H

#include "capability.h"
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

// Descriptor flags
#define DESC_FLAG_DMA_CAPABLE (1 << 0)
#define DESC_FLAG_SHARED      (1 << 1)
#define DESC_FLAG_PINNED      (1 << 2)

/**
 * @brief Memory descriptor structure
 */
typedef struct {
    // Memory reference
    void* ptr;
    size_t length;
    
    // Capability (access control)
    capability_t capability;
    
    // Metadata
    uint64_t descriptor_id;
    uint32_t flags;
    
    // Ownership tracking
    uint32_t owner_core;
    atomic_uint32_t ref_count;
} memory_descriptor_t;

/**
 * @brief Create a memory descriptor
 * 
 * @param ptr Pointer to memory
 * @param length Length of memory region
 * @param capability Capability for access control
 * @param flags Descriptor flags
 * @param owner_core Owner core ID
 * @return Memory descriptor
 */
memory_descriptor_t descriptor_create(void* ptr,
                                      size_t length,
                                      capability_t capability,
                                      uint32_t flags,
                                      uint32_t owner_core);

/**
 * @brief Validate descriptor
 * 
 * Checks capability and bounds.
 * 
 * @param desc Descriptor to validate
 * @param required_perms Required permissions
 * @return true if valid, false otherwise
 */
bool descriptor_validate(const memory_descriptor_t* desc, uint32_t required_perms);

/**
 * @brief Acquire reference to descriptor
 * 
 * Increments reference count.
 * 
 * @param desc Descriptor
 */
void descriptor_acquire(memory_descriptor_t* desc);

/**
 * @brief Release reference to descriptor
 * 
 * Decrements reference count.
 * 
 * @param desc Descriptor
 * @return true if descriptor should be freed (ref_count == 0)
 */
bool descriptor_release(memory_descriptor_t* desc);

/**
 * @brief Get reference count
 * 
 * @param desc Descriptor
 * @return Current reference count
 */
uint32_t descriptor_get_ref_count(const memory_descriptor_t* desc);

#ifdef __cplusplus
}
#endif

#endif // PHASE1_DESCRIPTOR_H
