// ===================================================================
// DESC: Unified device memory management with zero-copy support
//       and memory pooling for efficient device allocations.
// PHASE 13: Backend Acceleration - Day 2
// ===================================================================
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// C23 constexpr for memory management
constexpr size_t POOL_SIZE_4KB = 4096;
constexpr size_t POOL_SIZE_16KB = 16384;
constexpr size_t POOL_SIZE_64KB = 65536;
constexpr size_t POOL_SIZE_256KB = 262144;
constexpr int POOL_BLOCKS_PER_SIZE = 64;
constexpr size_t MAX_DEVICE_MEMORY = (4ULL * 1024 * 1024 * 1024); // 4GB

// Memory allocation flags
typedef enum {
    MEM_FLAG_NONE = 0,
    MEM_FLAG_ZERO_COPY = (1 << 0),      // Enable zero-copy transfer
    MEM_FLAG_UNIFIED = (1 << 1),        // CPU and device accessible
    MEM_FLAG_PINNED = (1 << 2),         // Pinned (non-pageable) memory
    MEM_FLAG_WRITE_COMBINED = (1 << 3), // Write-combined for faster transfers
    MEM_FLAG_CACHED = (1 << 4)          // Cached memory
} MemoryFlags;

// Memory pool block
typedef struct MemoryBlock {
    void* ptr;
    size_t size;
    _Atomic bool is_free;
    uint32_t flags;
    int device_id;
    struct MemoryBlock* next;
} MemoryBlock;

_Static_assert(sizeof(MemoryBlock) <= 64, "MemoryBlock structure too large");

// Memory pool for a specific size
typedef struct {
    size_t block_size;
    MemoryBlock blocks[POOL_BLOCKS_PER_SIZE];
    _Atomic int free_count;
    _Atomic int total_allocations;
} MemoryPool;

// Device memory state
typedef struct {
    int device_id;
    _Atomic size_t total_allocated;
    _Atomic size_t peak_allocated;
    _Atomic uint64_t allocation_count;
    _Atomic uint64_t free_count;
    MemoryPool pool_4kb;
    MemoryPool pool_16kb;
    MemoryPool pool_64kb;
    MemoryPool pool_256kb;
    _Atomic bool initialized;
} DeviceMemoryState;

// Global memory management state
typedef struct {
    _Atomic bool initialized;
    DeviceMemoryState devices[16]; // Support up to 16 devices
    _Atomic size_t total_system_allocated;
} MemoryManagerState;

static MemoryManagerState mem_manager = {
    .initialized = false,
    .total_system_allocated = 0
};

// ============================================================================
// Memory Pool Management
// ============================================================================

/**
 * Initialize a memory pool
 */
static void init_memory_pool(MemoryPool* pool, size_t block_size) {
    pool->block_size = block_size;
    atomic_store(&pool->free_count, POOL_BLOCKS_PER_SIZE);
    atomic_store(&pool->total_allocations, 0);
    
    for (int i = 0; i < POOL_BLOCKS_PER_SIZE; i++) {
        pool->blocks[i].ptr = nullptr;
        pool->blocks[i].size = block_size;
        atomic_store(&pool->blocks[i].is_free, true);
        pool->blocks[i].flags = 0;
        pool->blocks[i].device_id = -1;
        pool->blocks[i].next = nullptr;
    }
}

/**
 * Allocate from memory pool
 */
[[nodiscard]] static MemoryBlock* pool_alloc(MemoryPool* pool, int device_id, uint32_t flags) {
    // Find a free block
    for (int i = 0; i < POOL_BLOCKS_PER_SIZE; i++) {
        bool expected = true;
        if (atomic_compare_exchange_strong(&pool->blocks[i].is_free, &expected, false)) {
            MemoryBlock* block = &pool->blocks[i];
            
            // Allocate actual memory if not already allocated
            if (block->ptr == nullptr) {
                // In real implementation, use device-specific allocation
                // For simulation, use malloc with alignment
                posix_memalign(&block->ptr, 4096, pool->block_size);
                if (block->ptr == nullptr) {
                    atomic_store(&block->is_free, true);
                    return nullptr;
                }
                
                // Zero memory if requested
                if (flags & MEM_FLAG_ZERO_COPY) {
                    memset(block->ptr, 0, pool->block_size);
                }
            }
            
            block->device_id = device_id;
            block->flags = flags;
            atomic_fetch_sub(&pool->free_count, 1);
            atomic_fetch_add(&pool->total_allocations, 1);
            
            return block;
        }
    }
    
    return nullptr; // Pool exhausted
}

/**
 * Free block back to pool
 */
static void pool_free(MemoryPool* pool, MemoryBlock* block) {
    if (block == nullptr) return;
    
    block->device_id = -1;
    block->flags = 0;
    atomic_store(&block->is_free, true);
    atomic_fetch_add(&pool->free_count, 1);
}

// ============================================================================
// Device Memory Management
// ============================================================================

/**
 * Initialize memory manager
 */
[[nodiscard]] int backend_memory_init(void) {
    bool expected = false;
    if (!atomic_compare_exchange_strong(&mem_manager.initialized, &expected, true)) {
        return 0; // Already initialized
    }
    
    printf("BACKEND_MEMORY: Initializing device memory manager...\n");
    
    atomic_store(&mem_manager.total_system_allocated, 0);
    
    // Initialize all device memory states
    for (int i = 0; i < 16; i++) {
        DeviceMemoryState* dev = &mem_manager.devices[i];
        dev->device_id = i;
        atomic_store(&dev->total_allocated, 0);
        atomic_store(&dev->peak_allocated, 0);
        atomic_store(&dev->allocation_count, 0);
        atomic_store(&dev->free_count, 0);
        atomic_store(&dev->initialized, false);
        
        // Initialize memory pools
        init_memory_pool(&dev->pool_4kb, POOL_SIZE_4KB);
        init_memory_pool(&dev->pool_16kb, POOL_SIZE_16KB);
        init_memory_pool(&dev->pool_64kb, POOL_SIZE_64KB);
        init_memory_pool(&dev->pool_256kb, POOL_SIZE_256KB);
    }
    
    printf("BACKEND_MEMORY: Memory manager initialized.\n");
    return 0;
}

/**
 * Shutdown memory manager
 */
void backend_memory_shutdown(void) {
    bool expected = true;
    if (!atomic_compare_exchange_strong(&mem_manager.initialized, &expected, false)) {
        return; // Not initialized
    }
    
    printf("BACKEND_MEMORY: Shutting down memory manager...\n");
    
    // Free all allocated memory
    for (int dev_id = 0; dev_id < 16; dev_id++) {
        DeviceMemoryState* dev = &mem_manager.devices[dev_id];
        if (!atomic_load(&dev->initialized)) continue;
        
        // Free all pool blocks
        MemoryPool* pools[] = {&dev->pool_4kb, &dev->pool_16kb, 
                               &dev->pool_64kb, &dev->pool_256kb};
        
        for (int p = 0; p < 4; p++) {
            for (int i = 0; i < POOL_BLOCKS_PER_SIZE; i++) {
                if (pools[p]->blocks[i].ptr != nullptr) {
                    free(pools[p]->blocks[i].ptr);
                    pools[p]->blocks[i].ptr = nullptr;
                }
            }
        }
        
        printf("BACKEND_MEMORY: Device %d freed (peak: %zu bytes)\n",
               dev_id, atomic_load(&dev->peak_allocated));
    }
    
    printf("BACKEND_MEMORY: Total system memory used: %zu bytes\n",
           atomic_load(&mem_manager.total_system_allocated));
}

/**
 * Initialize device memory
 */
[[nodiscard]] int backend_memory_init_device(int device_id) {
    if (device_id < 0 || device_id >= 16) {
        return -1;
    }
    
    if (!atomic_load(&mem_manager.initialized)) {
        backend_memory_init();
    }
    
    DeviceMemoryState* dev = &mem_manager.devices[device_id];
    bool expected = false;
    if (!atomic_compare_exchange_strong(&dev->initialized, &expected, true)) {
        return 0; // Already initialized
    }
    
    printf("BACKEND_MEMORY: Initialized memory for device %d\n", device_id);
    return 0;
}

// ============================================================================
// Memory Allocation
// ============================================================================

/**
 * Allocate device memory
 */
[[nodiscard]] void* backend_alloc(int device_id, size_t size, uint32_t flags) {
    if (!atomic_load(&mem_manager.initialized)) {
        backend_memory_init();
    }
    
    if (device_id < 0 || device_id >= 16) {
        return nullptr;
    }
    
    DeviceMemoryState* dev = &mem_manager.devices[device_id];
    if (!atomic_load(&dev->initialized)) {
        backend_memory_init_device(device_id);
    }
    
    MemoryBlock* block = nullptr;
    
    // Try to allocate from appropriate pool
    if (size <= POOL_SIZE_4KB) {
        block = pool_alloc(&dev->pool_4kb, device_id, flags);
    } else if (size <= POOL_SIZE_16KB) {
        block = pool_alloc(&dev->pool_16kb, device_id, flags);
    } else if (size <= POOL_SIZE_64KB) {
        block = pool_alloc(&dev->pool_64kb, device_id, flags);
    } else if (size <= POOL_SIZE_256KB) {
        block = pool_alloc(&dev->pool_256kb, device_id, flags);
    }
    
    // If pool allocation failed or size too large, use direct allocation
    if (block == nullptr) {
        void* ptr = nullptr;
        posix_memalign(&ptr, 4096, size);
        if (ptr == nullptr) {
            return nullptr;
        }
        
        if (flags & MEM_FLAG_ZERO_COPY) {
            memset(ptr, 0, size);
        }
        
        atomic_fetch_add(&dev->total_allocated, size);
        atomic_fetch_add(&mem_manager.total_system_allocated, size);
        atomic_fetch_add(&dev->allocation_count, 1);
        
        size_t current = atomic_load(&dev->total_allocated);
        size_t peak = atomic_load(&dev->peak_allocated);
        if (current > peak) {
            atomic_store(&dev->peak_allocated, current);
        }
        
        printf("BACKEND_MEMORY: Direct alloc %zu bytes on device %d (flags=0x%x)\n",
               size, device_id, flags);
        return ptr;
    }
    
    // Pool allocation succeeded
    atomic_fetch_add(&dev->total_allocated, block->size);
    atomic_fetch_add(&mem_manager.total_system_allocated, block->size);
    atomic_fetch_add(&dev->allocation_count, 1);
    
    size_t current = atomic_load(&dev->total_allocated);
    size_t peak = atomic_load(&dev->peak_allocated);
    if (current > peak) {
        atomic_store(&dev->peak_allocated, current);
    }
    
    printf("BACKEND_MEMORY: Pool alloc %zu bytes on device %d (flags=0x%x)\n",
           block->size, device_id, flags);
    return block->ptr;
}

/**
 * Free device memory
 */
void backend_free(int device_id, void* ptr, size_t size) {
    if (ptr == nullptr || device_id < 0 || device_id >= 16) {
        return;
    }
    
    DeviceMemoryState* dev = &mem_manager.devices[device_id];
    
    // Try to find in pools
    MemoryPool* pools[] = {&dev->pool_4kb, &dev->pool_16kb, 
                           &dev->pool_64kb, &dev->pool_256kb};
    
    for (int p = 0; p < 4; p++) {
        for (int i = 0; i < POOL_BLOCKS_PER_SIZE; i++) {
            if (pools[p]->blocks[i].ptr == ptr) {
                pool_free(pools[p], &pools[p]->blocks[i]);
                atomic_fetch_sub(&dev->total_allocated, pools[p]->blocks[i].size);
                atomic_fetch_sub(&mem_manager.total_system_allocated, pools[p]->blocks[i].size);
                atomic_fetch_add(&dev->free_count, 1);
                printf("BACKEND_MEMORY: Pool free %zu bytes on device %d\n",
                       pools[p]->blocks[i].size, device_id);
                return;
            }
        }
    }
    
    // Not in pool, direct free
    free(ptr);
    atomic_fetch_sub(&dev->total_allocated, size);
    atomic_fetch_sub(&mem_manager.total_system_allocated, size);
    atomic_fetch_add(&dev->free_count, 1);
    printf("BACKEND_MEMORY: Direct free %zu bytes on device %d\n", size, device_id);
}

// ============================================================================
// Zero-Copy Data Transfer
// ============================================================================

/**
 * Zero-copy transfer from host to device
 * Uses memory mapping and DMA when available
 */
[[nodiscard]] int backend_transfer_h2d_zerocopy(int device_id, void* device_ptr,
                                                const void* host_ptr, size_t size) {
    if (device_ptr == nullptr || host_ptr == nullptr) {
        return -1;
    }
    
    printf("BACKEND_MEMORY: Zero-copy H2D transfer %zu bytes to device %d\n",
           size, device_id);
    
    // In real implementation, this would use DMA or memory mapping
    // For simulation, use direct memory copy
    memcpy(device_ptr, host_ptr, size);
    
    return 0;
}

/**
 * Zero-copy transfer from device to host
 */
[[nodiscard]] int backend_transfer_d2h_zerocopy(int device_id, void* host_ptr,
                                                const void* device_ptr, size_t size) {
    if (host_ptr == nullptr || device_ptr == nullptr) {
        return -1;
    }
    
    printf("BACKEND_MEMORY: Zero-copy D2H transfer %zu bytes from device %d\n",
           size, device_id);
    
    // In real implementation, this would use DMA or memory mapping
    // For simulation, use direct memory copy
    memcpy(host_ptr, device_ptr, size);
    
    return 0;
}

/**
 * Map device memory to host address space (unified memory)
 */
[[nodiscard]] void* backend_map_memory(int device_id, void* device_ptr, size_t size) {
    if (device_ptr == nullptr) {
        return nullptr;
    }
    
    printf("BACKEND_MEMORY: Mapping %zu bytes from device %d to host\n",
           size, device_id);
    
    // In real implementation, this would create a mapping
    // For simulation, return the same pointer (already accessible)
    return device_ptr;
}

/**
 * Unmap device memory from host address space
 */
void backend_unmap_memory(int device_id, void* mapped_ptr) {
    if (mapped_ptr == nullptr) {
        return;
    }
    
    printf("BACKEND_MEMORY: Unmapping memory from device %d\n", device_id);
    
    // In real implementation, this would remove the mapping
    // For simulation, no-op
}

// ============================================================================
// Memory Statistics
// ============================================================================

/**
 * Get memory statistics for a device
 */
void backend_memory_stats(int device_id) {
    if (device_id < 0 || device_id >= 16) {
        return;
    }
    
    DeviceMemoryState* dev = &mem_manager.devices[device_id];
    if (!atomic_load(&dev->initialized)) {
        printf("BACKEND_MEMORY: Device %d not initialized\n", device_id);
        return;
    }
    
    printf("\n=== Device %d Memory Statistics ===\n", device_id);
    printf("Total allocated: %zu bytes\n", atomic_load(&dev->total_allocated));
    printf("Peak allocated: %zu bytes\n", atomic_load(&dev->peak_allocated));
    printf("Allocations: %llu\n", (unsigned long long)atomic_load(&dev->allocation_count));
    printf("Frees: %llu\n", (unsigned long long)atomic_load(&dev->free_count));
    
    printf("\nMemory Pools:\n");
    printf("  4KB pool: %d/%d free\n", 
           atomic_load(&dev->pool_4kb.free_count), POOL_BLOCKS_PER_SIZE);
    printf("  16KB pool: %d/%d free\n",
           atomic_load(&dev->pool_16kb.free_count), POOL_BLOCKS_PER_SIZE);
    printf("  64KB pool: %d/%d free\n",
           atomic_load(&dev->pool_64kb.free_count), POOL_BLOCKS_PER_SIZE);
    printf("  256KB pool: %d/%d free\n",
           atomic_load(&dev->pool_256kb.free_count), POOL_BLOCKS_PER_SIZE);
    printf("=====================================\n\n");
}

/**
 * Get global memory statistics
 */
void backend_memory_global_stats(void) {
    if (!atomic_load(&mem_manager.initialized)) {
        printf("BACKEND_MEMORY: Memory manager not initialized\n");
        return;
    }
    
    printf("\n=== Global Memory Statistics ===\n");
    printf("Total system allocated: %zu bytes\n",
           atomic_load(&mem_manager.total_system_allocated));
    
    printf("\nPer-Device Summary:\n");
    for (int i = 0; i < 16; i++) {
        DeviceMemoryState* dev = &mem_manager.devices[i];
        if (atomic_load(&dev->initialized)) {
            printf("  Device %d: %zu bytes (peak: %zu)\n",
                   i, atomic_load(&dev->total_allocated),
                   atomic_load(&dev->peak_allocated));
        }
    }
    printf("=================================\n\n");
}
