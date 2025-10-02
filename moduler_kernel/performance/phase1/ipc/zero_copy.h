
/**
 * @file zero_copy.h
 * @brief Zero-copy transfer primitives
 * 
 * Provides high-level interface for zero-copy data transfer using descriptors.
 */

#ifndef PHASE1_ZERO_COPY_H
#define PHASE1_ZERO_COPY_H

#include "descriptor.h"

#ifdef __cplusplus
extern "C" {
#endif

// Zero-copy status codes
typedef enum {
    ZERO_COPY_SUCCESS = 0,
    ZERO_COPY_ERROR_INVALID_PARAM = -1,
    ZERO_COPY_ERROR_PERMISSION_DENIED = -2,
    ZERO_COPY_ERROR_OUT_OF_BOUNDS = -3
} zero_copy_status_t;

/**
 * @brief Send descriptor via zero-copy
 * 
 * Transfers ownership of memory descriptor without copying data.
 * 
 * @param desc Descriptor to send
 * @param target_core Target core ID
 * @return Status code
 */
zero_copy_status_t zero_copy_send(memory_descriptor_t* desc, uint32_t target_core);

/**
 * @brief Receive descriptor via zero-copy
 * 
 * Receives memory descriptor and validates access.
 * 
 * @param desc Output parameter for received descriptor
 * @param required_perms Required permissions
 * @return Status code
 */
zero_copy_status_t zero_copy_receive(memory_descriptor_t* desc, uint32_t required_perms);

/**
 * @brief Map descriptor for access
 * 
 * Validates descriptor and returns pointer for direct access.
 * 
 * @param desc Descriptor
 * @param required_perms Required permissions
 * @param ptr Output parameter for pointer
 * @param length Output parameter for length
 * @return Status code
 */
zero_copy_status_t zero_copy_map(const memory_descriptor_t* desc,
                                  uint32_t required_perms,
                                  void** ptr,
                                  size_t* length);

/**
 * @brief Unmap descriptor
 * 
 * Releases access to descriptor.
 * 
 * @param desc Descriptor
 * @return Status code
 */
zero_copy_status_t zero_copy_unmap(memory_descriptor_t* desc);

#ifdef __cplusplus
}
#endif

#endif // PHASE1_ZERO_COPY_H
