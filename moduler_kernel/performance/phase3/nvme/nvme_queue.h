
/**
 * @file nvme_queue.h
 * @brief NVMe queue pair management (submission/completion queues)
 * 
 * Lock-free, polling-based queue pair implementation for ultra-low latency I/O.
 */

#ifndef PHASE3_NVME_QUEUE_H
#define PHASE3_NVME_QUEUE_H

#include "nvme_device.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// NVMe command structure (64 bytes)
typedef struct nvme_command {
    uint32_t cdw0;      // Command Dword 0 (opcode, flags, CID)
    uint32_t nsid;      // Namespace ID
    uint64_t rsvd;      // Reserved
    uint64_t mptr;      // Metadata pointer
    uint64_t prp1;      // PRP Entry 1
    uint64_t prp2;      // PRP Entry 2
    uint32_t cdw10;     // Command-specific
    uint32_t cdw11;
    uint32_t cdw12;
    uint32_t cdw13;
    uint32_t cdw14;
    uint32_t cdw15;
} __attribute__((packed)) nvme_command_t;

// NVMe completion entry (16 bytes)
typedef struct nvme_completion {
    uint32_t result;    // Command-specific result
    uint32_t rsvd;      // Reserved
    uint16_t sq_head;   // Submission queue head pointer
    uint16_t sq_id;     // Submission queue identifier
    uint16_t cid;       // Command identifier
    uint16_t status;    // Status field (phase bit + status code)
} __attribute__((packed)) nvme_completion_t;

// Queue pair structure
typedef struct nvme_qpair {
    nvme_device_t* dev;
    
    // Queue IDs
    uint16_t qid;
    
    // Submission queue
    nvme_command_t* sq;
    uint16_t sq_size;
    uint16_t sq_head;
    uint16_t sq_tail;
    uint32_t sq_doorbell_offset;
    
    // Completion queue
    nvme_completion_t* cq;
    uint16_t cq_size;
    uint16_t cq_head;
    uint8_t cq_phase;
    uint32_t cq_doorbell_offset;
    
    // Outstanding commands
    void** cmd_ctx;     // Command context array (indexed by CID)
    uint16_t num_outstanding;
    
    // Statistics
    uint64_t submissions;
    uint64_t completions;
    uint64_t errors;
    
    // NUMA node
    int numa_node;
} nvme_qpair_t;

// I/O completion callback
typedef void (*nvme_io_completion_cb)(void* ctx, const nvme_completion_t* cpl);

/**
 * @brief Create admin queue pair
 * 
 * @param dev Device handle
 * @param sq_size Submission queue size
 * @param cq_size Completion queue size
 * @return Queue pair handle on success, NULL on failure
 */
nvme_qpair_t* nvme_create_admin_qpair(nvme_device_t* dev, 
                                      uint16_t sq_size, uint16_t cq_size);

/**
 * @brief Create I/O queue pair
 * 
 * @param dev Device handle
 * @param qid Queue ID (1-based)
 * @param sq_size Submission queue size
 * @param cq_size Completion queue size
 * @return Queue pair handle on success, NULL on failure
 */
nvme_qpair_t* nvme_create_io_qpair(nvme_device_t* dev, uint16_t qid,
                                   uint16_t sq_size, uint16_t cq_size);

/**
 * @brief Destroy queue pair
 * 
 * @param qpair Queue pair handle
 */
void nvme_destroy_qpair(nvme_qpair_t* qpair);

/**
 * @brief Submit command to queue
 * 
 * @param qpair Queue pair handle
 * @param cmd Command to submit
 * @param ctx Command context (passed to completion callback)
 * @return Command ID on success, negative on error
 */
int nvme_submit_command(nvme_qpair_t* qpair, const nvme_command_t* cmd, void* ctx);

/**
 * @brief Process completions (polling)
 * 
 * @param qpair Queue pair handle
 * @param max_completions Maximum completions to process (0 = all available)
 * @param cb Completion callback
 * @return Number of completions processed
 */
int nvme_process_completions(nvme_qpair_t* qpair, uint32_t max_completions,
                             nvme_io_completion_cb cb);

/**
 * @brief Get number of free submission queue entries
 * 
 * @param qpair Queue pair handle
 * @return Number of free entries
 */
static inline uint16_t nvme_qpair_get_free_entries(const nvme_qpair_t* qpair) {
    uint16_t used = (qpair->sq_tail - qpair->sq_head) & (qpair->sq_size - 1);
    return qpair->sq_size - used - 1; // Reserve one entry
}

/**
 * @brief Check if queue pair is full
 * 
 * @param qpair Queue pair handle
 * @return true if full, false otherwise
 */
static inline bool nvme_qpair_is_full(const nvme_qpair_t* qpair) {
    return nvme_qpair_get_free_entries(qpair) == 0;
}

/**
 * @brief Get queue pair statistics
 * 
 * @param qpair Queue pair handle
 * @param submissions Output total submissions
 * @param completions Output total completions
 * @param errors Output total errors
 */
void nvme_qpair_get_stats(const nvme_qpair_t* qpair, uint64_t* submissions,
                          uint64_t* completions, uint64_t* errors);

#ifdef __cplusplus
}
#endif

#endif // PHASE3_NVME_QUEUE_H
