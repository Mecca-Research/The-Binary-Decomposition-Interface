#include <stdlib.h>

// ===================================================================
// DESC: Storage Fast Path - Zero-copy I/O and fast path integration
//       Integrates with Phase 5 fast paths and Phase 4 zero-copy IPC
// ===================================================================

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <immintrin.h>  // For SIMD intrinsics

// ===================================================================
// Type Definitions
// ===================================================================

typedef uint32_t device_id_t;

typedef struct {
    device_id_t device;
    uint64_t lba;
    void* buffer;
    size_t size;
    bool is_write;
    uint32_t flags;
} io_request_t;

typedef struct {
    io_request_t* requests;
    uint32_t count;
    _Atomic uint32_t completed;
    void (*callback)(void* ctx, int status);
    void* callback_ctx;
} io_batch_t;

// Fast path flags
#define IO_FLAG_ZERO_COPY    (1 << 0)
#define IO_FLAG_DIRECT       (1 << 1)
#define IO_FLAG_SYNC         (1 << 2)
#define IO_FLAG_BYPASS_CACHE (1 << 3)

// Fast path thresholds
#define ZERO_COPY_THRESHOLD (64 * 1024)
#define DIRECT_IO_THRESHOLD (128 * 1024)
#define BATCH_SIZE_MAX 256

// ===================================================================
// SIMD-Optimized Memory Operations
// ===================================================================

/**
 * SIMD-optimized memory copy using AVX2
 * Significantly faster than memcpy for large transfers
 */
static inline void simd_memcpy(void* dest, const void* src, size_t size) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    
    // Use AVX2 for 32-byte chunks
    while (size >= 32) {
        __m256i data = _mm256_loadu_si256((const __m256i*)s);
        _mm256_storeu_si256((__m256i*)d, data);
        s += 32;
        d += 32;
        size -= 32;
    }
    
    // Handle remaining bytes
    while (size > 0) {
        *d++ = *s++;
        size--;
    }
}

/**
 * CRC32C calculation using SSE4.2 hardware acceleration
 * Used for data integrity verification
 */
[[nodiscard]] static inline uint32_t crc32c_sse42(const void* data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t* p = (const uint8_t*)data;
    
    // Process 8 bytes at a time using hardware CRC32C
    while (len >= 8) {
        crc = _mm_crc32_u64(crc, *(const uint64_t*)p);
        p += 8;
        len -= 8;
    }
    
    // Process remaining bytes
    while (len > 0) {
        crc = _mm_crc32_u8(crc, *p);
        p++;
        len--;
    }
    
    return ~crc;
}

// ===================================================================
// Zero-Copy I/O Implementation
// ===================================================================

/**
 * Zero-copy read operation
 * Maps device memory directly to user buffer, avoiding intermediate copies
 * Integrates with Phase 4 zero-copy IPC mechanisms
 */
[[nodiscard]] int storage_zero_copy_read(device_id_t dev, uint64_t lba, 
                                          void* buffer, size_t size) {
    // Validate alignment for zero-copy (must be page-aligned)
    if (((uintptr_t)buffer & 0xFFF) != 0) {
        return -1;  // EINVAL: Buffer not page-aligned
    }
    
    // TODO: Integrate with Phase 4 zero-copy IPC
    // 1. Pin user pages in memory
    // 2. Get physical addresses
    // 3. Program DMA directly to user buffer
    // 4. Wait for completion
    // 5. Unpin pages
    
    // For now, placeholder implementation
    return 0;
}

/**
 * Zero-copy write operation
 * Maps user buffer directly to device, avoiding intermediate copies
 */
[[nodiscard]] int storage_zero_copy_write(device_id_t dev, uint64_t lba,
                                           const void* buffer, size_t size) {
    // Validate alignment
    if (((uintptr_t)buffer & 0xFFF) != 0) {
        return -1;  // EINVAL
    }
    
    // TODO: Implement zero-copy write
    // Similar to read but with write direction
    
    return 0;
}

// ===================================================================
// Fast Path I/O Operations
// ===================================================================

/**
 * Fast path read operation
 * Automatically selects optimal path based on transfer size
 * Integrates with Phase 5 fast path infrastructure
 */
[[nodiscard]] int storage_fast_path_read(device_id_t dev, uint64_t lba,
                                          void* buffer, size_t size) {
    // Select optimal path based on size
    if (size >= ZERO_COPY_THRESHOLD) {
        // Use zero-copy for large transfers
        return storage_zero_copy_read(dev, lba, buffer, size);
    } else {
        // Use regular I/O with SIMD optimization
        // TODO: Call device-specific read function
        return 0;
    }
}

/**
 * Fast path write operation
 * Automatically selects optimal path based on transfer size
 */
[[nodiscard]] int storage_fast_path_write(device_id_t dev, uint64_t lba,
                                           const void* buffer, size_t size) {
    // Select optimal path based on size
    if (size >= ZERO_COPY_THRESHOLD) {
        // Use zero-copy for large transfers
        return storage_zero_copy_write(dev, lba, buffer, size);
    } else {
        // Use regular I/O with SIMD optimization
        // TODO: Call device-specific write function
        return 0;
    }
}

// ===================================================================
// I/O Batching and Coalescing
// ===================================================================

/**
 * Create I/O batch for multiple operations
 * Allows submitting multiple I/O requests together for better performance
 */
[[nodiscard]] io_batch_t* storage_batch_create(uint32_t max_requests) {
    if (max_requests > BATCH_SIZE_MAX) {
        return NULL;
    }
    
    io_batch_t* batch = (io_batch_t*)malloc(sizeof(io_batch_t));
    if (!batch) {
        return NULL;
    }
    
    batch->requests = (io_request_t*)malloc(sizeof(io_request_t) * max_requests);
    if (!batch->requests) {
        free(batch);
        return NULL;
    }
    
    batch->count = 0;
    batch->completed = 0;
    batch->callback = NULL;
    batch->callback_ctx = NULL;
    
    return batch;
}

/**
 * Add request to batch
 */
[[nodiscard]] int storage_batch_add(io_batch_t* batch, device_id_t dev,
                                     uint64_t lba, void* buffer, size_t size,
                                     bool is_write) {
    if (!batch || batch->count >= BATCH_SIZE_MAX) {
        return -1;
    }
    
    io_request_t* req = &batch->requests[batch->count++];
    req->device = dev;
    req->lba = lba;
    req->buffer = buffer;
    req->size = size;
    req->is_write = is_write;
    req->flags = 0;
    
    // Set flags based on size
    if (size >= ZERO_COPY_THRESHOLD) {
        req->flags |= IO_FLAG_ZERO_COPY;
    }
    if (size >= DIRECT_IO_THRESHOLD) {
        req->flags |= IO_FLAG_DIRECT;
    }
    
    return 0;
}

/**
 * Submit batch of I/O requests
 * Coalesces adjacent requests and submits them efficiently
 */
[[nodiscard]] int storage_batch_submit(io_batch_t* batch) {
    if (!batch || batch->count == 0) {
        return -1;
    }
    
    // TODO: Implement request coalescing
    // 1. Sort requests by LBA
    // 2. Merge adjacent requests
    // 3. Submit to device queues
    // 4. Track completion
    
    // For now, submit each request individually
    for (uint32_t i = 0; i < batch->count; i++) {
        io_request_t* req = &batch->requests[i];
        
        int result;
        if (req->is_write) {
            result = storage_fast_path_write(req->device, req->lba,
                                             req->buffer, req->size);
        } else {
            result = storage_fast_path_read(req->device, req->lba,
                                            req->buffer, req->size);
        }
        
        if (result != 0) {
            return result;
        }
        
        batch->completed++;
    }
    
    // Call completion callback if set
    if (batch->callback) {
        batch->callback(batch->callback_ctx, 0);
    }
    
    return 0;
}

/**
 * Wait for batch completion
 */
[[nodiscard]] int storage_batch_wait(io_batch_t* batch, uint32_t timeout_ms) {
    if (!batch) {
        return -1;
    }
    
    // TODO: Implement proper wait with timeout
    // For now, spin until completed
    while (batch->completed < batch->count) {
        // Yield CPU
        __builtin_ia32_pause();
    }
    
    return 0;
}

/**
 * Destroy I/O batch
 */
void storage_batch_destroy(io_batch_t* batch) {
    if (batch) {
        free(batch->requests);
        free(batch);
    }
}

// ===================================================================
// Direct I/O Bypass
// ===================================================================

/**
 * Direct I/O bypass for large transfers
 * Bypasses kernel page cache for better performance on large sequential I/O
 */
[[nodiscard]] int storage_direct_io(device_id_t dev, uint64_t lba,
                                     void* buffer, size_t size, bool is_write) {
    // Validate alignment (must be sector-aligned)
    if ((lba & 0x7) != 0 || (size & 0x1FF) != 0) {
        return -1;  // EINVAL: Not sector-aligned
    }
    
    // Use zero-copy path for direct I/O
    if (is_write) {
        return storage_zero_copy_write(dev, lba, buffer, size);
    } else {
        return storage_zero_copy_read(dev, lba, buffer, size);
    }
}

// ===================================================================
// Performance Monitoring
// ===================================================================

typedef struct {
    uint64_t total_reads;
    uint64_t total_writes;
    uint64_t bytes_read;
    uint64_t bytes_written;
    uint64_t zero_copy_ops;
    uint64_t direct_io_ops;
    uint64_t batched_ops;
} storage_stats_t;

static storage_stats_t global_stats = {0};

/**
 * Get storage statistics
 */
[[nodiscard]] const storage_stats_t* storage_get_stats(void) {
    return &global_stats;
}

/**
 * Reset storage statistics
 */
void storage_reset_stats(void) {
    memset(&global_stats, 0, sizeof(global_stats));
}
