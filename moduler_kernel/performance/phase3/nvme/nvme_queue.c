
/**
 * @file nvme_queue.c
 * @brief NVMe queue pair management implementation
 */

#include "nvme_queue.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

// Helper to ring doorbell
static inline void nvme_ring_doorbell(nvme_device_t* dev, uint32_t offset, uint32_t value) {
    volatile uint32_t* doorbell = (volatile uint32_t*)((char*)dev->bar0 + offset);
    *doorbell = value;
}

nvme_qpair_t* nvme_create_admin_qpair(nvme_device_t* dev,
                                      uint16_t sq_size, uint16_t cq_size) {
    if (!dev || sq_size == 0 || cq_size == 0) {
        return NULL;
    }
    
    // Allocate queue pair structure
    nvme_qpair_t* qpair = calloc(1, sizeof(nvme_qpair_t));
    if (!qpair) {
        return NULL;
    }
    
    qpair->dev = dev;
    qpair->qid = 0; // Admin queue ID is 0
    qpair->sq_size = sq_size;
    qpair->cq_size = cq_size;
    qpair->cq_phase = 1;
    qpair->numa_node = dev->numa_node;
    
    // Allocate submission queue (aligned to page boundary)
    qpair->sq = aligned_alloc(4096, sq_size * sizeof(nvme_command_t));
    if (!qpair->sq) {
        free(qpair);
        return NULL;
    }
    memset(qpair->sq, 0, sq_size * sizeof(nvme_command_t));
    
    // Allocate completion queue (aligned to page boundary)
    qpair->cq = aligned_alloc(4096, cq_size * sizeof(nvme_completion_t));
    if (!qpair->cq) {
        free(qpair->sq);
        free(qpair);
        return NULL;
    }
    memset(qpair->cq, 0, cq_size * sizeof(nvme_completion_t));
    
    // Allocate command context array
    qpair->cmd_ctx = calloc(sq_size, sizeof(void*));
    if (!qpair->cmd_ctx) {
        free(qpair->cq);
        free(qpair->sq);
        free(qpair);
        return NULL;
    }
    
    // Calculate doorbell offsets
    qpair->sq_doorbell_offset = 0x1000; // Admin SQ doorbell
    qpair->cq_doorbell_offset = 0x1000 + dev->caps.doorbell_stride; // Admin CQ doorbell
    
    return qpair;
}

nvme_qpair_t* nvme_create_io_qpair(nvme_device_t* dev, uint16_t qid,
                                   uint16_t sq_size, uint16_t cq_size) {
    if (!dev || qid == 0 || sq_size == 0 || cq_size == 0) {
        return NULL;
    }
    
    // Allocate queue pair structure
    nvme_qpair_t* qpair = calloc(1, sizeof(nvme_qpair_t));
    if (!qpair) {
        return NULL;
    }
    
    qpair->dev = dev;
    qpair->qid = qid;
    qpair->sq_size = sq_size;
    qpair->cq_size = cq_size;
    qpair->cq_phase = 1;
    qpair->numa_node = dev->numa_node;
    
    // Allocate submission queue
    qpair->sq = aligned_alloc(4096, sq_size * sizeof(nvme_command_t));
    if (!qpair->sq) {
        free(qpair);
        return NULL;
    }
    memset(qpair->sq, 0, sq_size * sizeof(nvme_command_t));
    
    // Allocate completion queue
    qpair->cq = aligned_alloc(4096, cq_size * sizeof(nvme_completion_t));
    if (!qpair->cq) {
        free(qpair->sq);
        free(qpair);
        return NULL;
    }
    memset(qpair->cq, 0, cq_size * sizeof(nvme_completion_t));
    
    // Allocate command context array
    qpair->cmd_ctx = calloc(sq_size, sizeof(void*));
    if (!qpair->cmd_ctx) {
        free(qpair->cq);
        free(qpair->sq);
        free(qpair);
        return NULL;
    }
    
    // Calculate doorbell offsets
    qpair->sq_doorbell_offset = 0x1000 + (2 * qid * dev->caps.doorbell_stride);
    qpair->cq_doorbell_offset = 0x1000 + ((2 * qid + 1) * dev->caps.doorbell_stride);
    
    return qpair;
}

void nvme_destroy_qpair(nvme_qpair_t* qpair) {
    if (!qpair) {
        return;
    }
    
    free(qpair->cmd_ctx);
    free(qpair->cq);
    free(qpair->sq);
    free(qpair);
}

int nvme_submit_command(nvme_qpair_t* qpair, const nvme_command_t* cmd, void* ctx) {
    if (!qpair || !cmd) {
        return -EINVAL;
    }
    
    // Check if queue is full
    if (nvme_qpair_is_full(qpair)) {
        return -EAGAIN;
    }
    
    // Get command ID from tail position
    uint16_t cid = qpair->sq_tail;
    
    // Copy command to submission queue
    memcpy(&qpair->sq[qpair->sq_tail], cmd, sizeof(nvme_command_t));
    
    // Set command ID in command
    qpair->sq[qpair->sq_tail].cdw0 = (qpair->sq[qpair->sq_tail].cdw0 & 0xFFFF0000) | cid;
    
    // Store command context
    qpair->cmd_ctx[cid] = ctx;
    
    // Advance tail
    qpair->sq_tail = (qpair->sq_tail + 1) & (qpair->sq_size - 1);
    qpair->num_outstanding++;
    qpair->submissions++;
    
    // Ring doorbell
    nvme_ring_doorbell(qpair->dev, qpair->sq_doorbell_offset, qpair->sq_tail);
    
    return cid;
}

int nvme_process_completions(nvme_qpair_t* qpair, uint32_t max_completions,
                             nvme_io_completion_cb cb) {
    if (!qpair) {
        return -EINVAL;
    }
    
    uint32_t num_completions = 0;
    
    // Process completions until phase bit mismatch or max reached
    while (num_completions < max_completions || max_completions == 0) {
        nvme_completion_t* cpl = &qpair->cq[qpair->cq_head];
        
        // Check phase bit
        uint8_t phase = (cpl->status >> 0) & 1;
        if (phase != qpair->cq_phase) {
            break; // No more completions
        }
        
        // Extract command ID
        uint16_t cid = cpl->cid;
        
        // Get command context
        void* ctx = qpair->cmd_ctx[cid];
        
        // Call completion callback
        if (cb) {
            cb(ctx, cpl);
        }
        
        // Clear command context
        qpair->cmd_ctx[cid] = NULL;
        
        // Update statistics
        qpair->completions++;
        qpair->num_outstanding--;
        
        if ((cpl->status >> 1) & 0x7FF) { // Check status code
            qpair->errors++;
        }
        
        // Advance head
        qpair->cq_head = (qpair->cq_head + 1) & (qpair->cq_size - 1);
        
        // Flip phase if we wrapped around
        if (qpair->cq_head == 0) {
            qpair->cq_phase = !qpair->cq_phase;
        }
        
        num_completions++;
    }
    
    // Ring completion queue doorbell if we processed any completions
    if (num_completions > 0) {
        nvme_ring_doorbell(qpair->dev, qpair->cq_doorbell_offset, qpair->cq_head);
    }
    
    return num_completions;
}

void nvme_qpair_get_stats(const nvme_qpair_t* qpair, uint64_t* submissions,
                          uint64_t* completions, uint64_t* errors) {
    if (!qpair) {
        return;
    }
    
    if (submissions) *submissions = qpair->submissions;
    if (completions) *completions = qpair->completions;
    if (errors) *errors = qpair->errors;
}
