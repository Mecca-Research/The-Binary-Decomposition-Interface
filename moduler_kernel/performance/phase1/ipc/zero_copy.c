
/**
 * @file zero_copy.c
 * @brief Implementation of zero-copy transfer primitives
 */

#include "zero_copy.h"

zero_copy_status_t zero_copy_send(memory_descriptor_t* desc, uint32_t target_core) {
    if (!desc) {
        return ZERO_COPY_ERROR_INVALID_PARAM;
    }
    
    // Validate descriptor
    if (!descriptor_validate(desc, CAP_PERM_READ)) {
        return ZERO_COPY_ERROR_PERMISSION_DENIED;
    }
    
    // Transfer ownership (simplified - in real implementation, use ring buffer)
    desc->owner_core = target_core;
    
    return ZERO_COPY_SUCCESS;
}

zero_copy_status_t zero_copy_receive(memory_descriptor_t* desc, uint32_t required_perms) {
    if (!desc) {
        return ZERO_COPY_ERROR_INVALID_PARAM;
    }
    
    // Validate descriptor
    if (!descriptor_validate(desc, required_perms)) {
        return ZERO_COPY_ERROR_PERMISSION_DENIED;
    }
    
    // Acquire reference
    descriptor_acquire(desc);
    
    return ZERO_COPY_SUCCESS;
}

zero_copy_status_t zero_copy_map(const memory_descriptor_t* desc,
                                  uint32_t required_perms,
                                  void** ptr,
                                  size_t* length) {
    if (!desc || !ptr || !length) {
        return ZERO_COPY_ERROR_INVALID_PARAM;
    }
    
    // Validate descriptor
    if (!descriptor_validate(desc, required_perms)) {
        return ZERO_COPY_ERROR_PERMISSION_DENIED;
    }
    
    // Return pointer and length (no actual mapping needed - already in shared memory)
    *ptr = desc->ptr;
    *length = desc->length;
    
    return ZERO_COPY_SUCCESS;
}

zero_copy_status_t zero_copy_unmap(memory_descriptor_t* desc) {
    if (!desc) {
        return ZERO_COPY_ERROR_INVALID_PARAM;
    }
    
    // Release reference
    descriptor_release(desc);
    
    return ZERO_COPY_SUCCESS;
}
