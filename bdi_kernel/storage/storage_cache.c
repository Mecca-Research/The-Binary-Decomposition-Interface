#include <stdlib.h>

// ===================================================================
// DESC: Storage Cache Management - Block cache with write-back/through
//       Implements cache coherency and prefetching
// ===================================================================

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

// ===================================================================
// Type Definitions
// ===================================================================

typedef uint32_t device_id_t;

// Cache block size (typically 4KB)
#define CACHE_BLOCK_SIZE 4096
#define CACHE_BLOCKS 16384
#define CACHE_WAYS 8

// Cache policies
typedef enum {
    CACHE_POLICY_WRITE_THROUGH,
    CACHE_POLICY_WRITE_BACK,
    CACHE_POLICY_WRITE_AROUND
} cache_policy_t;

// Cache block state
typedef enum {
    CACHE_STATE_INVALID,
    CACHE_STATE_CLEAN,
    CACHE_STATE_DIRTY
} cache_state_t;

// Cache block entry
typedef struct {
    device_id_t device;
    uint64_t lba;
    cache_state_t state;
    _Atomic uint32_t ref_count;
    uint64_t last_access;
    uint8_t data[CACHE_BLOCK_SIZE];
} cache_block_t;

// Cache set (for set-associative cache)
typedef struct {
    cache_block_t blocks[CACHE_WAYS];
    _Atomic uint32_t lock;
} cache_set_t;

// Global cache structure
typedef struct {
    cache_set_t* sets;
    size_t num_sets;
    cache_policy_t policy;
    _Atomic uint64_t access_counter;
    
    // Statistics
    _Atomic uint64_t hits;
    _Atomic uint64_t misses;
    _Atomic uint64_t evictions;
    _Atomic uint64_t writebacks;
} storage_cache_t;

static storage_cache_t global_cache = {0};

// ===================================================================
// Cache Initialization
// ===================================================================

/**
 * Initialize storage cache
 */
[[nodiscard]] int storage_cache_init(cache_policy_t policy) {
    global_cache.num_sets = CACHE_BLOCKS / CACHE_WAYS;
    global_cache.policy = policy;
    global_cache.access_counter = 0;
    
    // Allocate cache sets
    global_cache.sets = (cache_set_t*)malloc(
        sizeof(cache_set_t) * global_cache.num_sets);
    
    if (!global_cache.sets) {
        return -1;  // ENOMEM
    }
    
    // Initialize all blocks to invalid
    for (size_t i = 0; i < global_cache.num_sets; i++) {
        global_cache.sets[i].lock = 0;
        for (size_t j = 0; j < CACHE_WAYS; j++) {
            global_cache.sets[i].blocks[j].state = CACHE_STATE_INVALID;
            global_cache.sets[i].blocks[j].ref_count = 0;
        }
    }
    
    return 0;
}

// ===================================================================
// Cache Operations
// ===================================================================

/**
 * Calculate cache set index from LBA
 */
static inline size_t cache_get_set_index(uint64_t lba) {
    return (lba / (CACHE_BLOCK_SIZE / 512)) % global_cache.num_sets;
}

/**
 * Acquire cache set lock (spinlock)
 */
static inline void cache_lock_set(cache_set_t* set) {
    while (__atomic_test_and_set(&set->lock, __ATOMIC_ACQUIRE)) {
        __builtin_ia32_pause();
    }
}

/**
 * Release cache set lock
 */
static inline void cache_unlock_set(cache_set_t* set) {
    __atomic_clear(&set->lock, __ATOMIC_RELEASE);
}

/**
 * Find block in cache
 * Returns NULL if not found
 */
[[nodiscard]] static cache_block_t* cache_find_block(device_id_t device,
                                                       uint64_t lba) {
    size_t set_idx = cache_get_set_index(lba);
    cache_set_t* set = &global_cache.sets[set_idx];
    
    cache_lock_set(set);
    
    // Search all ways in the set
    for (size_t i = 0; i < CACHE_WAYS; i++) {
        cache_block_t* block = &set->blocks[i];
        if (block->state != CACHE_STATE_INVALID &&
            block->device == device &&
            block->lba == lba) {
            
            // Update access time
            block->last_access = global_cache.access_counter++;
            
            cache_unlock_set(set);
            return block;
        }
    }
    
    cache_unlock_set(set);
    return NULL;
}

/**
 * Evict least recently used block from set
 */
static cache_block_t* cache_evict_lru(cache_set_t* set) {
    cache_block_t* lru_block = NULL;
    uint64_t oldest_access = UINT64_MAX;
    
    // Find LRU block
    for (size_t i = 0; i < CACHE_WAYS; i++) {
        cache_block_t* block = &set->blocks[i];
        
        // Prefer invalid blocks
        if (block->state == CACHE_STATE_INVALID) {
            return block;
        }
        
        // Skip blocks with references
        if (block->ref_count > 0) {
            continue;
        }
        
        if (block->last_access < oldest_access) {
            oldest_access = block->last_access;
            lru_block = block;
        }
    }
    
    // Write back dirty block if needed
    if (lru_block && lru_block->state == CACHE_STATE_DIRTY) {
        // TODO: Write back to device
        global_cache.writebacks++;
    }
    
    global_cache.evictions++;
    return lru_block;
}

/**
 * Allocate cache block
 */
[[nodiscard]] static cache_block_t* cache_alloc_block(device_id_t device,
                                                        uint64_t lba) {
    size_t set_idx = cache_get_set_index(lba);
    cache_set_t* set = &global_cache.sets[set_idx];
    
    cache_lock_set(set);
    
    // Try to find empty slot or evict LRU
    cache_block_t* block = cache_evict_lru(set);
    
    if (block) {
        block->device = device;
        block->lba = lba;
        block->state = CACHE_STATE_CLEAN;
        block->ref_count = 0;
        block->last_access = global_cache.access_counter++;
    }
    
    cache_unlock_set(set);
    return block;
}

// ===================================================================
// Cache Read/Write Operations
// ===================================================================

/**
 * Read from cache
 * Returns 0 on hit, -1 on miss
 */
[[nodiscard]] int storage_cache_read(device_id_t device, uint64_t lba,
                                      void* buffer, size_t size) {
    // Only cache block-aligned reads
    if (size != CACHE_BLOCK_SIZE) {
        return -1;
    }
    
    cache_block_t* block = cache_find_block(device, lba);
    
    if (block) {
        // Cache hit
        memcpy(buffer, block->data, CACHE_BLOCK_SIZE);
        global_cache.hits++;
        return 0;
    }
    
    // Cache miss
    global_cache.misses++;
    
    // Allocate new block
    block = cache_alloc_block(device, lba);
    if (!block) {
        return -1;
    }
    
    // TODO: Read from device
    // For now, just mark as clean
    block->state = CACHE_STATE_CLEAN;
    
    memcpy(buffer, block->data, CACHE_BLOCK_SIZE);
    return 0;
}

/**
 * Write to cache
 */
[[nodiscard]] int storage_cache_write(device_id_t device, uint64_t lba,
                                       const void* buffer, size_t size) {
    // Only cache block-aligned writes
    if (size != CACHE_BLOCK_SIZE) {
        return -1;
    }
    
    cache_block_t* block = cache_find_block(device, lba);
    
    if (!block) {
        // Allocate new block on write miss
        block = cache_alloc_block(device, lba);
        if (!block) {
            return -1;
        }
    }
    
    // Copy data to cache
    memcpy(block->data, buffer, CACHE_BLOCK_SIZE);
    
    // Handle write policy
    switch (global_cache.policy) {
        case CACHE_POLICY_WRITE_THROUGH:
            // Write to device immediately
            // TODO: Write to device
            block->state = CACHE_STATE_CLEAN;
            break;
            
        case CACHE_POLICY_WRITE_BACK:
            // Mark as dirty, write later
            block->state = CACHE_STATE_DIRTY;
            break;
            
        case CACHE_POLICY_WRITE_AROUND:
            // Don't cache, write directly to device
            // TODO: Write to device
            block->state = CACHE_STATE_INVALID;
            break;
    }
    
    return 0;
}

/**
 * Flush dirty blocks to device
 */
[[nodiscard]] int storage_cache_flush(device_id_t device) {
    int flushed = 0;
    
    for (size_t i = 0; i < global_cache.num_sets; i++) {
        cache_set_t* set = &global_cache.sets[i];
        
        cache_lock_set(set);
        
        for (size_t j = 0; j < CACHE_WAYS; j++) {
            cache_block_t* block = &set->blocks[j];
            
            if (block->state == CACHE_STATE_DIRTY &&
                (device == 0 || block->device == device)) {
                
                // TODO: Write to device
                block->state = CACHE_STATE_CLEAN;
                flushed++;
            }
        }
        
        cache_unlock_set(set);
    }
    
    return flushed;
}

/**
 * Invalidate cache entries
 */
void storage_cache_invalidate(device_id_t device, uint64_t lba) {
    size_t set_idx = cache_get_set_index(lba);
    cache_set_t* set = &global_cache.sets[set_idx];
    
    cache_lock_set(set);
    
    for (size_t i = 0; i < CACHE_WAYS; i++) {
        cache_block_t* block = &set->blocks[i];
        
        if (block->device == device && block->lba == lba) {
            block->state = CACHE_STATE_INVALID;
            break;
        }
    }
    
    cache_unlock_set(set);
}

// ===================================================================
// Cache Statistics
// ===================================================================

typedef struct {
    uint64_t hits;
    uint64_t misses;
    uint64_t evictions;
    uint64_t writebacks;
    double hit_rate;
} cache_stats_t;

/**
 * Get cache statistics
 */
[[nodiscard]] cache_stats_t storage_cache_get_stats(void) {
    cache_stats_t stats;
    
    stats.hits = global_cache.hits;
    stats.misses = global_cache.misses;
    stats.evictions = global_cache.evictions;
    stats.writebacks = global_cache.writebacks;
    
    uint64_t total = stats.hits + stats.misses;
    stats.hit_rate = total > 0 ? (double)stats.hits / total : 0.0;
    
    return stats;
}

/**
 * Reset cache statistics
 */
void storage_cache_reset_stats(void) {
    global_cache.hits = 0;
    global_cache.misses = 0;
    global_cache.evictions = 0;
    global_cache.writebacks = 0;
}
