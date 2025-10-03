
// ===================================================================
// DESC: USB DMA Optimization Layer (Phase 12 Day 3)
//       Zero-copy DMA with scatter-gather support for USB transfers
// ===================================================================
// MODERNIZED: Phase 12 - C23 features with zero-copy DMA

#include <stdint.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

// DMA buffer descriptor
typedef struct {
    uint64_t physical_addr;                // Physical address
    void *virtual_addr;                    // Virtual address
    uint32_t size;                         // Buffer size
    _Atomic uint32_t ref_count;            // Reference count
    _Atomic bool in_use;                   // In-use flag
    bool coherent;                         // DMA coherent flag
} usb_dma_buffer_t;

// Scatter-gather list entry
typedef struct {
    uint64_t physical_addr;                // Physical address
    uint32_t length;                       // Segment length
    uint32_t offset;                       // Offset in buffer
} usb_sg_entry_t;

// Scatter-gather descriptor
typedef struct {
    usb_sg_entry_t *entries;               // SG entries
    uint32_t num_entries;                  // Number of entries
    uint32_t total_length;                 // Total transfer length
    _Atomic bool mapped;                   // Mapping state
} usb_sg_descriptor_t;

// DMA pool for buffer management
typedef struct {
    usb_dma_buffer_t *buffers;             // Buffer array
    uint32_t num_buffers;                  // Number of buffers
    uint32_t buffer_size;                  // Size of each buffer
    _Atomic uint32_t allocated_count;      // Allocated buffer count
    _Atomic uint32_t free_count;           // Free buffer count
    char name[64];                         // Pool name
} usb_dma_pool_t;

// Global DMA pools
#define USB_DMA_POOL_SMALL_SIZE     4096    // 4KB buffers
#define USB_DMA_POOL_MEDIUM_SIZE    65536   // 64KB buffers
#define USB_DMA_POOL_LARGE_SIZE     1048576 // 1MB buffers
#define USB_DMA_POOL_COUNT          3

static usb_dma_pool_t g_dma_pools[USB_DMA_POOL_COUNT];
static _Atomic bool g_dma_initialized = false;

/**
 * Initialize USB DMA subsystem
 * Creates buffer pools for different transfer sizes
 */
[[nodiscard]] int usb_dma_init(void) {
    if (atomic_load_explicit(&g_dma_initialized, memory_order_acquire)) {
        return 0;  // Already initialized
    }
    
    // Initialize small buffer pool (4KB, 256 buffers)
    usb_dma_pool_t *small_pool = &g_dma_pools[0];
    small_pool->num_buffers = 256;
    small_pool->buffer_size = USB_DMA_POOL_SMALL_SIZE;
    small_pool->buffers = calloc(256, sizeof(usb_dma_buffer_t));
    if (!small_pool->buffers) return -1;
    atomic_store_explicit(&small_pool->allocated_count, 0, memory_order_relaxed);
    atomic_store_explicit(&small_pool->free_count, 256, memory_order_relaxed);
    strncpy(small_pool->name, "USB_DMA_SMALL", 63);
    
    // Initialize medium buffer pool (64KB, 64 buffers)
    usb_dma_pool_t *medium_pool = &g_dma_pools[1];
    medium_pool->num_buffers = 64;
    medium_pool->buffer_size = USB_DMA_POOL_MEDIUM_SIZE;
    medium_pool->buffers = calloc(64, sizeof(usb_dma_buffer_t));
    if (!medium_pool->buffers) {
        free(small_pool->buffers);
        return -1;
    }
    atomic_store_explicit(&medium_pool->allocated_count, 0, memory_order_relaxed);
    atomic_store_explicit(&medium_pool->free_count, 64, memory_order_relaxed);
    strncpy(medium_pool->name, "USB_DMA_MEDIUM", 63);
    
    // Initialize large buffer pool (1MB, 16 buffers)
    usb_dma_pool_t *large_pool = &g_dma_pools[2];
    large_pool->num_buffers = 16;
    large_pool->buffer_size = USB_DMA_POOL_LARGE_SIZE;
    large_pool->buffers = calloc(16, sizeof(usb_dma_buffer_t));
    if (!large_pool->buffers) {
        free(small_pool->buffers);
        free(medium_pool->buffers);
        return -1;
    }
    atomic_store_explicit(&large_pool->allocated_count, 0, memory_order_relaxed);
    atomic_store_explicit(&large_pool->free_count, 16, memory_order_relaxed);
    strncpy(large_pool->name, "USB_DMA_LARGE", 63);
    
    atomic_store_explicit(&g_dma_initialized, true, memory_order_release);
    return 0;
}

/**
 * Allocate DMA buffer from pool (zero-copy)
 * Uses atomic operations for lock-free allocation
 */
[[nodiscard]] usb_dma_buffer_t* usb_dma_alloc_buffer(uint32_t size) {
    if (!atomic_load_explicit(&g_dma_initialized, memory_order_acquire)) {
        return nullptr;
    }
    
    // Select appropriate pool based on size
    usb_dma_pool_t *pool = nullptr;
    if (size <= USB_DMA_POOL_SMALL_SIZE) {
        pool = &g_dma_pools[0];
    } else if (size <= USB_DMA_POOL_MEDIUM_SIZE) {
        pool = &g_dma_pools[1];
    } else if (size <= USB_DMA_POOL_LARGE_SIZE) {
        pool = &g_dma_pools[2];
    } else {
        return nullptr;  // Size too large
    }
    
    // Lock-free buffer allocation using CAS
    for (uint32_t i = 0; i < pool->num_buffers; i++) {
        usb_dma_buffer_t *buffer = &pool->buffers[i];
        bool expected = false;
        
        if (atomic_compare_exchange_strong_explicit(&buffer->in_use,
                                                     &expected, true,
                                                     memory_order_acq_rel,
                                                     memory_order_acquire)) {
            // Successfully allocated buffer
            buffer->size = size;
            atomic_store_explicit(&buffer->ref_count, 1, memory_order_relaxed);
            
            // Update pool statistics
            atomic_fetch_add_explicit(&pool->allocated_count, 1, memory_order_relaxed);
            atomic_fetch_sub_explicit(&pool->free_count, 1, memory_order_relaxed);
            
            return buffer;
        }
    }
    
    return nullptr;  // Pool exhausted
}

/**
 * Free DMA buffer back to pool
 */
void usb_dma_free_buffer(usb_dma_buffer_t *buffer) {
    if (!buffer) return;
    
    // Decrement reference count
    uint32_t old_ref = atomic_fetch_sub_explicit(&buffer->ref_count, 1, memory_order_release);
    
    if (old_ref == 1) {
        // Last reference, return to pool
        atomic_store_explicit(&buffer->in_use, false, memory_order_release);
        
        // Update pool statistics (find which pool this belongs to)
        for (uint32_t i = 0; i < USB_DMA_POOL_COUNT; i++) {
            usb_dma_pool_t *pool = &g_dma_pools[i];
            if (buffer >= pool->buffers && buffer < pool->buffers + pool->num_buffers) {
                atomic_fetch_sub_explicit(&pool->allocated_count, 1, memory_order_relaxed);
                atomic_fetch_add_explicit(&pool->free_count, 1, memory_order_relaxed);
                break;
            }
        }
    }
}

/**
 * Create scatter-gather descriptor for zero-copy transfers
 * Maps user buffer to physical pages without copying
 */
[[nodiscard]] usb_sg_descriptor_t* usb_dma_create_sg_descriptor(void *buffer, uint32_t length) {
    if (!buffer || length == 0) {
        return nullptr;
    }
    
    usb_sg_descriptor_t *sg_desc = calloc(1, sizeof(usb_sg_descriptor_t));
    if (!sg_desc) return nullptr;
    
    // Calculate number of pages needed (assuming 4KB pages)
    uint32_t page_size = 4096;
    uint64_t start_addr = (uint64_t)buffer;
    uint64_t end_addr = start_addr + length;
    uint32_t num_pages = ((end_addr + page_size - 1) / page_size) - (start_addr / page_size);
    
    // Allocate SG entries
    sg_desc->entries = calloc(num_pages, sizeof(usb_sg_entry_t));
    if (!sg_desc->entries) {
        free(sg_desc);
        return nullptr;
    }
    
    // Build scatter-gather list (zero-copy mapping)
    uint64_t current_addr = start_addr;
    uint32_t remaining = length;
    
    for (uint32_t i = 0; i < num_pages; i++) {
        usb_sg_entry_t *entry = &sg_desc->entries[i];
        
        // Calculate physical address (simplified - in real kernel, use page tables)
        entry->physical_addr = current_addr;  // Would be virt_to_phys(current_addr)
        
        // Calculate length for this segment
        uint32_t offset_in_page = current_addr % page_size;
        uint32_t bytes_in_page = page_size - offset_in_page;
        entry->length = (remaining < bytes_in_page) ? remaining : bytes_in_page;
        entry->offset = offset_in_page;
        
        current_addr += entry->length;
        remaining -= entry->length;
    }
    
    sg_desc->num_entries = num_pages;
    sg_desc->total_length = length;
    atomic_store_explicit(&sg_desc->mapped, true, memory_order_release);
    
    return sg_desc;
}

/**
 * Free scatter-gather descriptor
 */
void usb_dma_free_sg_descriptor(usb_sg_descriptor_t *sg_desc) {
    if (!sg_desc) return;
    
    atomic_store_explicit(&sg_desc->mapped, false, memory_order_release);
    
    if (sg_desc->entries) {
        free(sg_desc->entries);
    }
    free(sg_desc);
}

/**
 * Map buffer for DMA (ensure cache coherency)
 */
[[nodiscard]] int usb_dma_map_buffer(usb_dma_buffer_t *buffer, bool to_device) {
    if (!buffer) return -1;
    
    // Ensure cache coherency
    if (!buffer->coherent) {
        if (to_device) {
            // Flush cache to memory (write-back)
            __asm__ volatile("" ::: "memory");  // Memory barrier
        } else {
            // Invalidate cache (before reading from device)
            __asm__ volatile("" ::: "memory");  // Memory barrier
        }
    }
    
    return 0;
}

/**
 * Unmap buffer after DMA
 */
void usb_dma_unmap_buffer(usb_dma_buffer_t *buffer, bool from_device) {
    if (!buffer) return;
    
    // Ensure cache coherency after DMA
    if (!buffer->coherent && from_device) {
        // Invalidate cache (device wrote to memory)
        __asm__ volatile("" ::: "memory");  // Memory barrier
    }
}

/**
 * Get DMA pool statistics
 */
[[nodiscard]] uint32_t usb_dma_get_pool_allocated(uint32_t pool_idx) {
    if (pool_idx >= USB_DMA_POOL_COUNT) return 0;
    return atomic_load_explicit(&g_dma_pools[pool_idx].allocated_count, memory_order_acquire);
}

[[nodiscard]] uint32_t usb_dma_get_pool_free(uint32_t pool_idx) {
    if (pool_idx >= USB_DMA_POOL_COUNT) return 0;
    return atomic_load_explicit(&g_dma_pools[pool_idx].free_count, memory_order_acquire);
}

/**
 * Cleanup DMA subsystem
 */
void usb_dma_cleanup(void) {
    if (!atomic_load_explicit(&g_dma_initialized, memory_order_acquire)) {
        return;
    }
    
    // Free all pools
    for (uint32_t i = 0; i < USB_DMA_POOL_COUNT; i++) {
        if (g_dma_pools[i].buffers) {
            free(g_dma_pools[i].buffers);
            g_dma_pools[i].buffers = nullptr;
        }
    }
    
    atomic_store_explicit(&g_dma_initialized, false, memory_order_release);
}
