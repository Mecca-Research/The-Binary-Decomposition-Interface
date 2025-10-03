
/**
 * @file nvme.c
 * @brief NVMe driver implementation with polling queues
 * 
 * Phase 5: Storage I/O Fast Paths
 * 
 * This driver implements high-performance NVMe I/O using:
 * - Polling queues (no interrupts) from Phase 3
 * - Zero-copy I/O from Phase 4
 * - C23 atomics for MMIO
 * - I/O batching and coalescing
 */

#include "nvme.h"
#include <string.h>
#include <stdlib.h>

/* Debug flag - can be disabled in production */
[[maybe_unused]] static int nvme_debug = 0;

/**
 * @brief Debug print helper
 */
[[maybe_unused]] static void nvme_debug_print(const char *fmt, ...) {
    #ifdef NVME_DEBUG
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    #endif
}

/**
 * @brief Wait for controller ready
 * @param ctrl Controller
 * @param ready 1 to wait for ready, 0 to wait for not ready
 * @return 0 on success, -ETIMEDOUT on timeout
 */
static int nvme_wait_ready(struct nvme_ctrl *ctrl, int ready) {
    uint32_t csts;
    int timeout = 5000; /* 5 seconds */
    
    while (timeout-- > 0) {
        csts = nvme_read_reg32((volatile char *)ctrl->bar + NVME_REG_CSTS);
        if (((csts & NVME_CSTS_RDY) != 0) == ready) {
            return 0;
        }
        /* Sleep for 1ms - in real kernel would use proper delay */
        usleep(1000);
    }
    
    return -ETIMEDOUT;
}

/**
 * @brief Initialize queue pair
 * @param q Queue to initialize
 * @param qid Queue ID
 * @param depth Queue depth
 * @param doorbell_base Doorbell base address
 * @param doorbell_stride Doorbell stride
 * @return 0 on success, negative error code on failure
 */
static int nvme_init_queue(struct nvme_queue *q, uint16_t qid, uint16_t depth,
                           volatile void *doorbell_base, uint32_t doorbell_stride) {
    /* Allocate submission queue */
    q->sq = aligned_alloc(4096, depth * sizeof(struct nvme_command));
    if (!q->sq) {
        return -ENOMEM;
    }
    memset(q->sq, 0, depth * sizeof(struct nvme_command));
    
    /* Allocate completion queue */
    q->cq = aligned_alloc(4096, depth * sizeof(struct nvme_completion));
    if (!q->cq) {
        free(q->sq);
        return -ENOMEM;
    }
    memset(q->cq, 0, depth * sizeof(struct nvme_completion));
    
    /* Initialize queue state */
    atomic_init(&q->sq_head, 0);
    atomic_init(&q->sq_tail, 0);
    atomic_init(&q->cq_head, 0);
    q->cq_phase = 1;
    q->qid = qid;
    q->depth = depth;
    
    /* Calculate doorbell addresses */
    q->sq_doorbell = (volatile char *)doorbell_base + (2 * qid) * doorbell_stride;
    q->cq_doorbell = (volatile char *)doorbell_base + (2 * qid + 1) * doorbell_stride;
    
    return 0;
}

/**
 * @brief Initialize NVMe controller
 */
int nvme_init(struct nvme_ctrl *ctrl, volatile void *bar) {
    uint64_t cap;
    uint32_t cc, aqa;
    int ret;
    
    if (!ctrl || !bar) {
        return -EINVAL;
    }
    
    memset(ctrl, 0, sizeof(*ctrl));
    ctrl->bar = bar;
    
    /* Read controller capabilities */
    cap = nvme_read_reg64((volatile char *)bar + NVME_REG_CAP);
    ctrl->cap = cap;
    
    /* Extract doorbell stride (in bytes) */
    ctrl->doorbell_stride = (4 << ((cap >> 32) & 0xF));
    
    /* Extract page size */
    uint32_t mpsmin = (cap >> 48) & 0xF;
    ctrl->page_size = 1 << (12 + mpsmin);
    
    /* Disable controller */
    cc = nvme_read_reg32((volatile char *)bar + NVME_REG_CC);
    cc &= ~NVME_CC_ENABLE;
    nvme_write_reg32((volatile char *)bar + NVME_REG_CC, cc);
    
    /* Wait for controller to be disabled */
    ret = nvme_wait_ready(ctrl, 0);
    if (ret) {
        return ret;
    }
    
    /* Initialize admin queue */
    ret = nvme_init_queue(&ctrl->admin_q, 0, 64,
                         (volatile char *)bar + 0x1000, ctrl->doorbell_stride);
    if (ret) {
        return ret;
    }
    
    /* Set admin queue attributes */
    aqa = ((ctrl->admin_q.depth - 1) << 16) | (ctrl->admin_q.depth - 1);
    nvme_write_reg32((volatile char *)bar + NVME_REG_AQA, aqa);
    
    /* Set admin queue addresses */
    nvme_write_reg64((volatile char *)bar + NVME_REG_ASQ, (uint64_t)ctrl->admin_q.sq);
    nvme_write_reg64((volatile char *)bar + NVME_REG_ACQ, (uint64_t)ctrl->admin_q.cq);
    
    /* Enable controller */
    cc = nvme_read_reg32((volatile char *)bar + NVME_REG_CC);
    cc |= NVME_CC_ENABLE;
    cc |= (6 << NVME_CC_IOSQES_SHIFT);  /* 64-byte submission entries */
    cc |= (4 << NVME_CC_IOCQES_SHIFT);  /* 16-byte completion entries */
    nvme_write_reg32((volatile char *)bar + NVME_REG_CC, cc);
    
    /* Wait for controller to be ready */
    ret = nvme_wait_ready(ctrl, 1);
    if (ret) {
        free(ctrl->admin_q.sq);
        free(ctrl->admin_q.cq);
        return ret;
    }
    
    /* Allocate I/O queues (simplified - just 1 for now) */
    ctrl->num_queues = 1;
    ctrl->io_queues = malloc(sizeof(struct nvme_queue) * ctrl->num_queues);
    if (!ctrl->io_queues) {
        free(ctrl->admin_q.sq);
        free(ctrl->admin_q.cq);
        return -ENOMEM;
    }
    
    /* Initialize I/O queue */
    ret = nvme_init_queue(&ctrl->io_queues[0], 1, 256,
                         (volatile char *)bar + 0x1000, ctrl->doorbell_stride);
    if (ret) {
        free(ctrl->io_queues);
        free(ctrl->admin_q.sq);
        free(ctrl->admin_q.cq);
        return ret;
    }
    
    /* TODO: Create I/O queues using admin commands */
    /* This would involve submitting CREATE_CQ and CREATE_SQ admin commands */
    
    ctrl->max_transfer_size = 128 * 1024; /* 128KB default */
    
    return 0;
}

/**
 * @brief Submit command to queue
 */
int nvme_submit_cmd(struct nvme_queue *q, struct nvme_command *cmd) {
    uint32_t tail, head;
    
    if (!q || !cmd) {
        return -EINVAL;
    }
    
    /* Get current tail and head */
    tail = atomic_load_explicit(&q->sq_tail, memory_order_acquire);
    head = atomic_load_explicit(&q->sq_head, memory_order_acquire);
    
    /* Check if queue is full */
    if (((tail + 1) % q->depth) == head) {
        return -ENOSPC;
    }
    
    /* Copy command to submission queue */
    memcpy(&q->sq[tail], cmd, sizeof(*cmd));
    
    /* Advance tail */
    tail = (tail + 1) % q->depth;
    atomic_store_explicit(&q->sq_tail, tail, memory_order_release);
    
    /* Ring doorbell (MMIO write) */
    nvme_write_reg32(q->sq_doorbell, tail);
    
    return 0;
}

/**
 * @brief Poll completion queue (no interrupts)
 */
int nvme_poll_cq(struct nvme_queue *q, struct nvme_completion *cpl) {
    uint32_t head;
    struct nvme_completion *entry;
    uint16_t status;
    
    if (!q || !cpl) {
        return -EINVAL;
    }
    
    /* Get current head */
    head = atomic_load_explicit(&q->cq_head, memory_order_acquire);
    entry = &q->cq[head];
    
    /* Check phase bit to see if entry is valid */
    status = atomic_load_explicit((_Atomic uint16_t *)&entry->status, memory_order_acquire);
    if ((status & 1) != q->cq_phase) {
        return -EAGAIN;  /* No completion yet */
    }
    
    /* Copy completion entry */
    memcpy(cpl, entry, sizeof(*cpl));
    
    /* Advance head */
    head++;
    if (head >= q->depth) {
        head = 0;
        q->cq_phase ^= 1;  /* Flip phase */
    }
    atomic_store_explicit(&q->cq_head, head, memory_order_release);
    
    /* Update submission queue head from completion */
    atomic_store_explicit(&q->sq_head, cpl->sq_head, memory_order_release);
    
    /* Ring doorbell */
    nvme_write_reg32(q->cq_doorbell, head);
    
    return 0;
}

/**
 * @brief Read from NVMe device (zero-copy)
 */
int nvme_read(struct nvme_ctrl *ctrl, uint32_t nsid, uint64_t lba, void *buf, size_t count) {
    struct nvme_command cmd = {0};
    struct nvme_completion cpl;
    int ret;
    
    if (!ctrl || !buf || count == 0) {
        return -EINVAL;
    }
    
    /* Use first I/O queue */
    struct nvme_queue *q = &ctrl->io_queues[0];
    
    /* Build read command */
    cmd.opcode = NVME_CMD_READ;
    cmd.nsid = nsid;
    cmd.prp1 = (uint64_t)buf;  /* Zero-copy: direct buffer address */
    cmd.cdw10 = lba & 0xFFFFFFFF;
    cmd.cdw11 = lba >> 32;
    cmd.cdw12 = (count - 1) & 0xFFFF;  /* Number of blocks - 1 */
    
    /* Submit command */
    ret = nvme_submit_cmd(q, &cmd);
    if (ret) {
        return ret;
    }
    
    /* Poll for completion */
    while (1) {
        ret = nvme_poll_cq(q, &cpl);
        if (ret == 0) {
            break;
        }
        if (ret != -EAGAIN) {
            return ret;
        }
        /* In real kernel, would yield CPU here */
    }
    
    /* Check completion status */
    if ((cpl.status >> 1) != NVME_SC_SUCCESS) {
        return -EIO;
    }
    
    return 0;
}

/**
 * @brief Write to NVMe device (zero-copy)
 */
int nvme_write(struct nvme_ctrl *ctrl, uint32_t nsid, uint64_t lba, const void *buf, size_t count) {
    struct nvme_command cmd = {0};
    struct nvme_completion cpl;
    int ret;
    
    if (!ctrl || !buf || count == 0) {
        return -EINVAL;
    }
    
    /* Use first I/O queue */
    struct nvme_queue *q = &ctrl->io_queues[0];
    
    /* Build write command */
    cmd.opcode = NVME_CMD_WRITE;
    cmd.nsid = nsid;
    cmd.prp1 = (uint64_t)buf;  /* Zero-copy: direct buffer address */
    cmd.cdw10 = lba & 0xFFFFFFFF;
    cmd.cdw11 = lba >> 32;
    cmd.cdw12 = (count - 1) & 0xFFFF;  /* Number of blocks - 1 */
    
    /* Submit command */
    ret = nvme_submit_cmd(q, &cmd);
    if (ret) {
        return ret;
    }
    
    /* Poll for completion */
    while (1) {
        ret = nvme_poll_cq(q, &cpl);
        if (ret == 0) {
            break;
        }
        if (ret != -EAGAIN) {
            return ret;
        }
    }
    
    /* Check completion status */
    if ((cpl.status >> 1) != NVME_SC_SUCCESS) {
        return -EIO;
    }
    
    return 0;
}

/**
 * @brief Submit batch of commands (I/O batching)
 */
int nvme_submit_batch(struct nvme_queue *q, struct nvme_command *cmds, size_t count) {
    uint32_t tail, head;
    size_t i;
    
    if (!q || !cmds || count == 0) {
        return -EINVAL;
    }
    
    /* Get current tail and head */
    tail = atomic_load_explicit(&q->sq_tail, memory_order_acquire);
    head = atomic_load_explicit(&q->sq_head, memory_order_acquire);
    
    /* Check if we have space for all commands */
    uint32_t available = (head > tail) ? (head - tail - 1) : (q->depth - tail + head - 1);
    if (available < count) {
        return -ENOSPC;
    }
    
    /* Copy all commands to submission queue */
    for (i = 0; i < count; i++) {
        memcpy(&q->sq[tail], &cmds[i], sizeof(struct nvme_command));
        tail = (tail + 1) % q->depth;
    }
    
    /* Update tail once for all commands */
    atomic_store_explicit(&q->sq_tail, tail, memory_order_release);
    
    /* Ring doorbell once for entire batch */
    nvme_write_reg32(q->sq_doorbell, tail);
    
    return 0;
}

/**
 * @brief Cleanup NVMe controller
 */
void nvme_cleanup(struct nvme_ctrl *ctrl) {
    uint32_t i;
    
    if (!ctrl) {
        return;
    }
    
    /* Disable controller */
    if (ctrl->bar) {
        uint32_t cc = nvme_read_reg32((volatile char *)ctrl->bar + NVME_REG_CC);
        cc &= ~NVME_CC_ENABLE;
        nvme_write_reg32((volatile char *)ctrl->bar + NVME_REG_CC, cc);
        nvme_wait_ready(ctrl, 0);
    }
    
    /* Free I/O queues */
    if (ctrl->io_queues) {
        for (i = 0; i < ctrl->num_queues; i++) {
            free(ctrl->io_queues[i].sq);
            free(ctrl->io_queues[i].cq);
        }
        free(ctrl->io_queues);
    }
    
    /* Free admin queue */
    free(ctrl->admin_q.sq);
    free(ctrl->admin_q.cq);
    
    memset(ctrl, 0, sizeof(*ctrl));
}
