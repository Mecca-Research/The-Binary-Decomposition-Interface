
/**
 * @file descriptor.c
 * @brief Implementation of memory descriptors
 */

#include "descriptor.h"
#include <string.h>
#include <time.h>

// Global descriptor ID counter
static atomic_uint64_t g_next_descriptor_id = 1;

capability_t capability_create(void* base_address,
                                size_t size,
                                uint32_t permissions,
                                cap_trust_level_t trust_level,
                                uint64_t valid_from,
                                uint64_t valid_until) {
    capability_t cap;
    static atomic_uint64_t next_id = 1;
    
    cap.capability_id = atomic_fetch_add(&next_id, 1);
    cap.permissions = permissions;
    cap.trust_level = trust_level;
    cap.base_address = base_address;
    cap.size = size;
    cap.valid_from = valid_from ? valid_from : capability_get_timestamp();
    cap.valid_until = valid_until;
    
    return cap;
}

bool capability_validate(const capability_t* cap,
                         void* ptr,
                         size_t size,
                         uint32_t required_perms) {
    if (!cap || !ptr) {
        return false;
    }
    
    // Check permissions
    if ((cap->permissions & required_perms) != required_perms) {
        return false;
    }
    
    // Check spatial bounds
    uintptr_t ptr_addr = (uintptr_t)ptr;
    uintptr_t base_addr = (uintptr_t)cap->base_address;
    uintptr_t end_addr = base_addr + cap->size;
    
    if (ptr_addr < base_addr || ptr_addr + size > end_addr) {
        return false;
    }
    
    // Check temporal bounds
    if (cap->valid_until > 0) {
        uint64_t now = capability_get_timestamp();
        if (now < cap->valid_from || now > cap->valid_until) {
            return false;
        }
    }
    
    return true;
}

bool capability_has_permission(const capability_t* cap, uint32_t perm) {
    return cap && (cap->permissions & perm) == perm;
}

uint64_t capability_get_timestamp(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

memory_descriptor_t descriptor_create(void* ptr,
                                      size_t length,
                                      capability_t capability,
                                      uint32_t flags,
                                      uint32_t owner_core) {
    memory_descriptor_t desc;
    
    desc.ptr = ptr;
    desc.length = length;
    desc.capability = capability;
    desc.descriptor_id = atomic_fetch_add(&g_next_descriptor_id, 1);
    desc.flags = flags;
    desc.owner_core = owner_core;
    atomic_init(&desc.ref_count, 1);
    
    return desc;
}

bool descriptor_validate(const memory_descriptor_t* desc, uint32_t required_perms) {
    if (!desc) {
        return false;
    }
    
    return capability_validate(&desc->capability, desc->ptr, desc->length, required_perms);
}

void descriptor_acquire(memory_descriptor_t* desc) {
    if (desc) {
        atomic_fetch_add(&desc->ref_count, 1);
    }
}

bool descriptor_release(memory_descriptor_t* desc) {
    if (!desc) {
        return false;
    }
    
    uint32_t old_count = atomic_fetch_sub(&desc->ref_count, 1);
    return old_count == 1;  // Should free if this was the last reference
}

uint32_t descriptor_get_ref_count(const memory_descriptor_t* desc) {
    if (!desc) {
        return 0;
    }
    
    return atomic_load(&desc->ref_count);
}
