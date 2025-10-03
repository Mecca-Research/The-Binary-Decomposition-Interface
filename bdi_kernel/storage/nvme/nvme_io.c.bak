
// ===================================================================
// DESC: NVMe I/O Command implementation for BDI Kernel
//       Handles NVMe I/O operations (read, write, flush, etc.)
// ===================================================================

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// NVMe I/O Command Opcodes
#define NVME_CMD_FLUSH          0x00
#define NVME_CMD_WRITE          0x01
#define NVME_CMD_READ           0x02
#define NVME_CMD_WRITE_UNCOR    0x04
#define NVME_CMD_COMPARE        0x05
#define NVME_CMD_WRITE_ZEROES   0x08
#define NVME_CMD_DSM            0x09
#define NVME_CMD_VERIFY         0x0C
#define NVME_CMD_RESV_REGISTER  0x0D
#define NVME_CMD_RESV_REPORT    0x0E
#define NVME_CMD_RESV_ACQUIRE   0x11
#define NVME_CMD_RESV_RELEASE   0x15

// NVMe Status Codes
#define NVME_SC_SUCCESS         0x00
#define NVME_SC_INVALID_OPCODE  0x01
#define NVME_SC_INVALID_FIELD   0x02
#define NVME_SC_CMDID_CONFLICT  0x03
#define NVME_SC_DATA_XFER_ERROR 0x04
#define NVME_SC_POWER_LOSS      0x05
#define NVME_SC_INTERNAL        0x06
#define NVME_SC_ABORT_REQ       0x07
#define NVME_SC_ABORT_QUEUE     0x08
#define NVME_SC_FUSED_FAIL      0x09
#define NVME_SC_FUSED_MISSING   0x0A
#define NVME_SC_INVALID_NS      0x0B
#define NVME_SC_LBA_RANGE       0x80
#define NVME_SC_CAP_EXCEEDED    0x81
#define NVME_SC_NS_NOT_READY    0x82

// NVMe I/O Command Structure
typedef struct {
    uint8_t opcode;         // Command opcode
    uint8_t flags;          // Command flags
    uint16_t command_id;    // Command identifier
    uint32_t nsid;          // Namespace identifier
    uint64_t rsvd2[2];      // Reserved
    uint64_t metadata;      // Metadata pointer
    uint64_t prp1;          // PRP entry 1
    uint64_t prp2;          // PRP entry 2
    uint64_t slba;          // Starting LBA (CDW10-11)
    uint32_t nlb;           // Number of logical blocks (CDW12)
    uint32_t control;       // Control flags (CDW13)
    uint32_t dsmgmt;        // Dataset management (CDW14)
    uint32_t reftag;        // Reference tag (CDW15)
} nvme_io_command_t;

// NVMe I/O Completion Entry
typedef struct {
    uint32_t result;        // Command-specific result
    uint32_t rsvd;          // Reserved
    uint16_t sq_head;       // Submission queue head pointer
    uint16_t sq_id;         // Submission queue identifier
    uint16_t command_id;    // Command identifier
    uint16_t status;        // Status field
} nvme_io_completion_t;

// I/O Queue Structure
typedef struct {
    uint16_t qid;           // Queue ID
    uint16_t size;          // Queue size
    uint16_t sq_tail;       // Submission queue tail
    uint16_t cq_head;       // Completion queue head
    uint8_t cq_phase;       // Completion queue phase bit
    nvme_io_command_t *sq;  // Submission queue
    nvme_io_completion_t *cq; // Completion queue
    uint32_t *sq_doorbell;  // Submission queue doorbell
    uint32_t *cq_doorbell;  // Completion queue doorbell
    uint8_t active;         // Queue active flag
} nvme_io_queue_t;

// Global I/O queue state
#define NVME_MAX_IO_QUEUES  16
static nvme_io_queue_t g_io_queues[NVME_MAX_IO_QUEUES];
static uint16_t g_io_command_id = 1;
static uint8_t g_io_initialized = 0;

// Function prototypes
int nvme_io_init(void);
int nvme_io_create_queue(uint16_t qid, uint16_t size, void *sq_mem, void *cq_mem,
                        uint32_t *sq_doorbell, uint32_t *cq_doorbell);
int nvme_io_delete_queue(uint16_t qid);
int nvme_io_submit_command(uint16_t qid, nvme_io_command_t *cmd);
int nvme_io_wait_completion(uint16_t qid, uint16_t command_id, nvme_io_completion_t *cpl);
int nvme_io_read(uint16_t qid, uint32_t nsid, uint64_t slba, uint32_t nlb, void *buffer);
int nvme_io_write(uint16_t qid, uint32_t nsid, uint64_t slba, uint32_t nlb, const void *buffer);
int nvme_io_flush(uint16_t qid, uint32_t nsid);
int nvme_io_write_zeroes(uint16_t qid, uint32_t nsid, uint64_t slba, uint32_t nlb);
int nvme_io_compare(uint16_t qid, uint32_t nsid, uint64_t slba, uint32_t nlb, const void *buffer);
int nvme_io_dsm(uint16_t qid, uint32_t nsid, uint32_t nr_ranges, void *ranges);
void nvme_io_cleanup(void);

/**
 * Initialize NVMe I/O subsystem
 */
int nvme_io_init(void) {
    if (g_io_initialized) {
        return 0;
    }
    
    // Clear I/O queue array
    memset(g_io_queues, 0, sizeof(g_io_queues));
    g_io_command_id = 1;
    g_io_initialized = 1;
    
    return 0;
}

/**
 * Create I/O queue pair
 */
int nvme_io_create_queue(uint16_t qid, uint16_t size, void *sq_mem, void *cq_mem,
                        uint32_t *sq_doorbell, uint32_t *cq_doorbell) {
    if (qid == 0 || qid >= NVME_MAX_IO_QUEUES || size == 0 || 
        !sq_mem || !cq_mem || !sq_doorbell || !cq_doorbell) {
        return -1;
    }
    
    nvme_io_queue_t *queue = &g_io_queues[qid];
    if (queue->active) {
        return -1; // Queue already exists
    }
    
    // Initialize queue structure
    queue->qid = qid;
    queue->size = size;
    queue->sq_tail = 0;
    queue->cq_head = 0;
    queue->cq_phase = 1;
    queue->sq = (nvme_io_command_t *)sq_mem;
    queue->cq = (nvme_io_completion_t *)cq_mem;
    queue->sq_doorbell = sq_doorbell;
    queue->cq_doorbell = cq_doorbell;
    queue->active = 1;
    
    return 0;
}

/**
 * Delete I/O queue
 */
int nvme_io_delete_queue(uint16_t qid) {
    if (qid == 0 || qid >= NVME_MAX_IO_QUEUES) {
        return -1;
    }
    
    nvme_io_queue_t *queue = &g_io_queues[qid];
    if (!queue->active) {
        return -1; // Queue doesn't exist
    }
    
    // Clear queue structure
    memset(queue, 0, sizeof(nvme_io_queue_t));
    
    return 0;
}

/**
 * Submit I/O command
 */
int nvme_io_submit_command(uint16_t qid, nvme_io_command_t *cmd) {
    if (qid == 0 || qid >= NVME_MAX_IO_QUEUES || !cmd) {
        return -1;
    }
    
    nvme_io_queue_t *queue = &g_io_queues[qid];
    if (!queue->active) {
        return -1;
    }
    
    // Assign command ID
    cmd->command_id = g_io_command_id++;
    if (g_io_command_id == 0) {
        g_io_command_id = 1; // Skip 0
    }
    
    // Copy command to submission queue
    memcpy(&queue->sq[queue->sq_tail], cmd, sizeof(nvme_io_command_t));
    
    // Update tail pointer
    queue->sq_tail = (queue->sq_tail + 1) % queue->size;
    
    // Ring doorbell
    *queue->sq_doorbell = queue->sq_tail;
    
    return cmd->command_id;
}

/**
 * Wait for I/O completion
 */
int nvme_io_wait_completion(uint16_t qid, uint16_t command_id, nvme_io_completion_t *cpl) {
    if (qid == 0 || qid >= NVME_MAX_IO_QUEUES || !cpl) {
        return -1;
    }
    
    nvme_io_queue_t *queue = &g_io_queues[qid];
    if (!queue->active) {
        return -1;
    }
    
    // In a real implementation, this would:
    // 1. Wait for interrupt or poll completion queue
    // 2. Check phase bit to detect new completions
    // 3. Match command ID and return completion entry
    // 4. Update completion queue head pointer
    
    // For simulation, create a successful completion
    memset(cpl, 0, sizeof(nvme_io_completion_t));
    cpl->command_id = command_id;
    cpl->status = NVME_SC_SUCCESS;
    cpl->sq_head = queue->sq_tail;
    cpl->sq_id = qid;
    
    // Update completion queue head
    queue->cq_head = (queue->cq_head + 1) % queue->size;
    if (queue->cq_head == 0) {
        queue->cq_phase = !queue->cq_phase;
    }
    
    // Ring completion doorbell
    *queue->cq_doorbell = queue->cq_head;
    
    return 0;
}

/**
 * Read data from NVMe namespace
 */
int nvme_io_read(uint16_t qid, uint32_t nsid, uint64_t slba, uint32_t nlb, void *buffer) {
    if (!buffer || nlb == 0) {
        return -1;
    }
    
    // Prepare read command
    nvme_io_command_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = NVME_CMD_READ;
    cmd.nsid = nsid;
    cmd.prp1 = (uint64_t)buffer;
    cmd.slba = slba;
    cmd.nlb = nlb - 1; // 0-based
    
    // Submit command
    int cmd_id = nvme_io_submit_command(qid, &cmd);
    if (cmd_id < 0) {
        return -1;
    }
    
    // Wait for completion
    nvme_io_completion_t cpl;
    if (nvme_io_wait_completion(qid, cmd_id, &cpl) != 0) {
        return -1;
    }
    
    // For simulation, fill buffer with pattern
    memset(buffer, 0xAA, nlb * 512); // Assume 512-byte blocks
    
    return (cpl.status & 0x7FF) == NVME_SC_SUCCESS ? 0 : -1;
}

/**
 * Write data to NVMe namespace
 */
int nvme_io_write(uint16_t qid, uint32_t nsid, uint64_t slba, uint32_t nlb, const void *buffer) {
    if (!buffer || nlb == 0) {
        return -1;
    }
    
    // Prepare write command
    nvme_io_command_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = NVME_CMD_WRITE;
    cmd.nsid = nsid;
    cmd.prp1 = (uint64_t)buffer;
    cmd.slba = slba;
    cmd.nlb = nlb - 1; // 0-based
    
    // Submit command
    int cmd_id = nvme_io_submit_command(qid, &cmd);
    if (cmd_id < 0) {
        return -1;
    }
    
    // Wait for completion
    nvme_io_completion_t cpl;
    if (nvme_io_wait_completion(qid, cmd_id, &cpl) != 0) {
        return -1;
    }
    
    return (cpl.status & 0x7FF) == NVME_SC_SUCCESS ? 0 : -1;
}

/**
 * Flush data to NVMe namespace
 */
int nvme_io_flush(uint16_t qid, uint32_t nsid) {
    // Prepare flush command
    nvme_io_command_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = NVME_CMD_FLUSH;
    cmd.nsid = nsid;
    
    // Submit command
    int cmd_id = nvme_io_submit_command(qid, &cmd);
    if (cmd_id < 0) {
        return -1;
    }
    
    // Wait for completion
    nvme_io_completion_t cpl;
    if (nvme_io_wait_completion(qid, cmd_id, &cpl) != 0) {
        return -1;
    }
    
    return (cpl.status & 0x7FF) == NVME_SC_SUCCESS ? 0 : -1;
}

/**
 * Write zeroes to NVMe namespace
 */
int nvme_io_write_zeroes(uint16_t qid, uint32_t nsid, uint64_t slba, uint32_t nlb) {
    if (nlb == 0) {
        return -1;
    }
    
    // Prepare write zeroes command
    nvme_io_command_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = NVME_CMD_WRITE_ZEROES;
    cmd.nsid = nsid;
    cmd.slba = slba;
    cmd.nlb = nlb - 1; // 0-based
    
    // Submit command
    int cmd_id = nvme_io_submit_command(qid, &cmd);
    if (cmd_id < 0) {
        return -1;
    }
    
    // Wait for completion
    nvme_io_completion_t cpl;
    if (nvme_io_wait_completion(qid, cmd_id, &cpl) != 0) {
        return -1;
    }
    
    return (cpl.status & 0x7FF) == NVME_SC_SUCCESS ? 0 : -1;
}

/**
 * Compare data in NVMe namespace
 */
int nvme_io_compare(uint16_t qid, uint32_t nsid, uint64_t slba, uint32_t nlb, const void *buffer) {
    if (!buffer || nlb == 0) {
        return -1;
    }
    
    // Prepare compare command
    nvme_io_command_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = NVME_CMD_COMPARE;
    cmd.nsid = nsid;
    cmd.prp1 = (uint64_t)buffer;
    cmd.slba = slba;
    cmd.nlb = nlb - 1; // 0-based
    
    // Submit command
    int cmd_id = nvme_io_submit_command(qid, &cmd);
    if (cmd_id < 0) {
        return -1;
    }
    
    // Wait for completion
    nvme_io_completion_t cpl;
    if (nvme_io_wait_completion(qid, cmd_id, &cpl) != 0) {
        return -1;
    }
    
    return (cpl.status & 0x7FF) == NVME_SC_SUCCESS ? 0 : -1;
}

/**
 * Dataset Management (TRIM/Deallocate)
 */
int nvme_io_dsm(uint16_t qid, uint32_t nsid, uint32_t nr_ranges, void *ranges) {
    if (!ranges || nr_ranges == 0) {
        return -1;
    }
    
    // Prepare dataset management command
    nvme_io_command_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = NVME_CMD_DSM;
    cmd.nsid = nsid;
    cmd.prp1 = (uint64_t)ranges;
    cmd.nlb = nr_ranges - 1; // 0-based number of ranges
    cmd.control = 0x4; // Deallocate attribute
    
    // Submit command
    int cmd_id = nvme_io_submit_command(qid, &cmd);
    if (cmd_id < 0) {
        return -1;
    }
    
    // Wait for completion
    nvme_io_completion_t cpl;
    if (nvme_io_wait_completion(qid, cmd_id, &cpl) != 0) {
        return -1;
    }
    
    return (cpl.status & 0x7FF) == NVME_SC_SUCCESS ? 0 : -1;
}

/**
 * Get I/O queue statistics
 */
int nvme_io_get_queue_stats(uint16_t qid, uint16_t *sq_tail, uint16_t *cq_head) {
    if (qid == 0 || qid >= NVME_MAX_IO_QUEUES) {
        return -1;
    }
    
    nvme_io_queue_t *queue = &g_io_queues[qid];
    if (!queue->active) {
        return -1;
    }
    
    if (sq_tail) {
        *sq_tail = queue->sq_tail;
    }
    if (cq_head) {
        *cq_head = queue->cq_head;
    }
    
    return 0;
}

/**
 * Check if I/O queue is active
 */
int nvme_io_queue_active(uint16_t qid) {
    if (qid == 0 || qid >= NVME_MAX_IO_QUEUES) {
        return 0;
    }
    
    return g_io_queues[qid].active;
}

/**
 * Cleanup I/O subsystem
 */
void nvme_io_cleanup(void) {
    // Clear all I/O queues
    memset(g_io_queues, 0, sizeof(g_io_queues));
    g_io_command_id = 1;
    g_io_initialized = 0;
}

// ===================================================================
// Phase 10: Lock-Free Queue Management and SIMD Optimizations
// ===================================================================

#include <immintrin.h>  // For SIMD intrinsics
#include <stdatomic.h>

// ===================================================================
// Lock-Free Submission Queue Operations
// ===================================================================

/**
 * Lock-free enqueue to submission queue
 * Uses atomic operations for thread-safe queue management
 */
[[nodiscard]] static inline int nvme_sq_enqueue_lockfree(nvme_sq_t* sq, 
                                                          const nvme_command_t* cmd) {
    if (!sq || !cmd) {
        return -1;
    }
    
    // Load current tail atomically
    uint32_t tail = atomic_load_explicit(&sq->tail, memory_order_acquire);
    uint32_t next_tail = (tail + 1) % sq->size;
    
    // Check if queue is full
    uint32_t head = atomic_load_explicit(&sq->head, memory_order_acquire);
    if (next_tail == head) {
        return -1;  // Queue full
    }
    
    // Copy command to queue
    memcpy(&sq->commands[tail], cmd, sizeof(nvme_command_t));
    
    // Memory barrier to ensure command is written before updating tail
    atomic_thread_fence(memory_order_release);
    
    // Update tail atomically
    atomic_store_explicit(&sq->tail, next_tail, memory_order_release);
    
    // Ring doorbell to notify device
    if (sq->doorbell) {
        *sq->doorbell = next_tail;
    }
    
    return 0;
}

/**
 * Lock-free dequeue from completion queue
 * Processes completions without locks
 */
[[nodiscard]] static inline int nvme_cq_dequeue_lockfree(nvme_cq_t* cq,
                                                          nvme_completion_t* comp) {
    if (!cq || !comp) {
        return -1;
    }
    
    // Load current head atomically
    uint32_t head = atomic_load_explicit(&cq->head, memory_order_acquire);
    
    // Check phase bit to see if entry is valid
    nvme_completion_t* entry = &cq->completions[head];
    uint16_t phase = (entry->status >> 15) & 1;
    
    if (phase != cq->phase) {
        return -1;  // No new completion
    }
    
    // Copy completion entry
    memcpy(comp, entry, sizeof(nvme_completion_t));
    
    // Update head atomically
    uint32_t next_head = (head + 1) % cq->size;
    atomic_store_explicit(&cq->head, next_head, memory_order_release);
    
    // Update phase if we wrapped around
    if (next_head == 0) {
        cq->phase = !cq->phase;
    }
    
    // Ring doorbell to acknowledge completion
    if (cq->doorbell) {
        *cq->doorbell = next_head;
    }
    
    return 0;
}

// ===================================================================
// SIMD-Optimized Data Operations
// ===================================================================

/**
 * SIMD-optimized memory copy for DMA buffers
 * Uses AVX2 for 32-byte transfers
 */
static inline void nvme_simd_memcpy(void* dest, const void* src, size_t size) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    
    // Use AVX2 for large transfers
    while (size >= 32) {
        __m256i data = _mm256_loadu_si256((const __m256i*)s);
        _mm256_storeu_si256((__m256i*)d, data);
        s += 32;
        d += 32;
        size -= 32;
    }
    
    // Use SSE for 16-byte chunks
    while (size >= 16) {
        __m128i data = _mm_loadu_si128((const __m128i*)s);
        _mm_storeu_si128((__m128i*)d, data);
        s += 16;
        d += 16;
        size -= 16;
    }
    
    // Handle remaining bytes
    while (size > 0) {
        *d++ = *s++;
        size--;
    }
}

/**
 * CRC32C calculation using SSE4.2 for data integrity
 */
[[nodiscard]] static inline uint32_t nvme_crc32c(const void* data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t* p = (const uint8_t*)data;
    
    // Process 8 bytes at a time
    while (len >= 8) {
        crc = _mm_crc32_u64(crc, *(const uint64_t*)p);
        p += 8;
        len -= 8;
    }
    
    // Process 4 bytes
    if (len >= 4) {
        crc = _mm_crc32_u32(crc, *(const uint32_t*)p);
        p += 4;
        len -= 4;
    }
    
    // Process remaining bytes
    while (len > 0) {
        crc = _mm_crc32_u8(crc, *p);
        p++;
        len--;
    }
    
    return ~crc;
}

/**
 * Vectorized DMA descriptor setup
 * Uses SIMD to initialize multiple descriptors at once
 */
static inline void nvme_setup_prp_list_simd(uint64_t* prp_list, 
                                            uint64_t base_addr,
                                            size_t num_pages) {
    const uint64_t page_size = 4096;
    
    // Use AVX2 to set up 4 PRPs at a time
    for (size_t i = 0; i < num_pages; i += 4) {
        __m256i addrs = _mm256_set_epi64x(
            base_addr + (i + 3) * page_size,
            base_addr + (i + 2) * page_size,
            base_addr + (i + 1) * page_size,
            base_addr + i * page_size
        );
        _mm256_storeu_si256((__m256i*)&prp_list[i], addrs);
    }
}

// ===================================================================
// Multi-Queue Support
// ===================================================================

/**
 * Get optimal I/O queue for current CPU
 * Implements per-CPU queue assignment for better cache locality
 */
[[nodiscard]] static inline uint16_t nvme_get_optimal_queue(void) {
    // TODO: Get current CPU ID
    // For now, return queue 1
    return 1;
}

/**
 * Submit I/O with automatic queue selection
 */
[[nodiscard]] int nvme_submit_io_multiqueue(nvme_device_t* dev,
                                             const nvme_command_t* cmd) {
    if (!dev || !cmd) {
        return -1;
    }
    
    // Select optimal queue based on CPU affinity
    uint16_t queue_id = nvme_get_optimal_queue();
    
    // Get the queue
    nvme_sq_t* sq = &dev->io_queues[queue_id];
    
    // Submit using lock-free enqueue
    return nvme_sq_enqueue_lockfree(sq, cmd);
}

// ===================================================================
// Interrupt Coalescing
// ===================================================================

/**
 * Configure interrupt coalescing for better performance
 * Reduces interrupt overhead by batching completions
 */
[[nodiscard]] int nvme_configure_interrupt_coalescing(nvme_device_t* dev,
                                                       uint8_t threshold,
                                                       uint8_t time_us) {
    if (!dev) {
        return -1;
    }
    
    // Set interrupt coalescing parameters
    uint32_t config = (threshold & 0xFF) | ((time_us & 0xFF) << 8);
    
    // TODO: Write to device registers
    // nvme_write_reg(dev, NVME_REG_INTC, config);
    
    return 0;
}

