
/**
 * @file arena_common.h
 * @brief Common definitions for shared memory arena
 */

#ifndef PHASE1_ARENA_COMMON_H
#define PHASE1_ARENA_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Arena status codes
typedef enum {
    ARENA_SUCCESS = 0,
    ARENA_ERROR_INVALID_PARAM = -1,
    ARENA_ERROR_OUT_OF_MEMORY = -2,
    ARENA_ERROR_ALLOCATION = -3,
    ARENA_ERROR_NOT_INITIALIZED = -4
} arena_status_t;

// Size classes for segregated free lists
#define ARENA_NUM_SIZE_CLASSES 12
static const size_t ARENA_SIZE_CLASSES[ARENA_NUM_SIZE_CLASSES] = {
    64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072
};

// DMA alignment (4KB for most hardware)
#define ARENA_DMA_ALIGNMENT 4096

// Arena statistics
typedef struct {
    uint64_t total_allocations;
    uint64_t total_frees;
    uint64_t current_allocated;
    uint64_t peak_allocated;
    uint64_t fragmentation_bytes;
} arena_stats_t;

#ifdef __cplusplus
}
#endif

#endif // PHASE1_ARENA_COMMON_H
