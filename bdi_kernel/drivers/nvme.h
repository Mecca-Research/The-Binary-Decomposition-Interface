
/**
 * @file nvme.h
 * @brief NVMe driver with polling queues and zero-copy I/O
 * 
 * Phase 5: Storage I/O Fast Paths
 * 
 * Key Features:
 * - Polling queues (no interrupts) - leverages Phase 3 polling techniques
 * - Direct I/O with zero-copy
 * - MMIO optimization with C23 atomics
 * - I/O batching and coalescing
 * - Admin queue and I/O queues
 * - NVMe command submission and completion
 */

#ifndef BDI_NVME_H
#define BDI_NVME_H

#include <stdint.h>
#include <stdatomic.h>
#include <stddef.h>
#include <errno.h>

/* NVMe command opcodes */
#define NVME_CMD_FLUSH          0x00
#define NVME_CMD_WRITE          0x01
#define NVME_CMD_READ           0x02

/* NVMe admin command opcodes */
#define NVME_ADMIN_DELETE_SQ    0x00
#define NVME_ADMIN_CREATE_SQ    0x01
#define NVME_ADMIN_DELETE_CQ    0x04
#define NVME_ADMIN_CREATE_CQ    0x05
#define NVME_ADMIN_IDENTIFY     0x06
#define NVME_ADMIN_SET_FEATURES 0x09
#define NVME_ADMIN_GET_FEATURES 0x0A

/* NVMe completion status */
#define NVME_SC_SUCCESS         0x00
#define NVME_SC_INVALID_OPCODE  0x01
#define NVME_SC_INVALID_FIELD   0x02

/* NVMe controller registers (offsets from BAR) */
#define NVME_REG_CAP            0x00  /* Controller Capabilities */
#define NVME_REG_VS             0x08  /* Version */
#define NVME_REG_CC             0x14  /* Controller Configuration */
#define NVME_REG_CSTS           0x1C  /* Controller Status */
#define NVME_REG_AQA            0x24  /* Admin Queue Attributes */
#define NVME_REG_ASQ            0x28  /* Admin Submission Queue Base */
#define NVME_REG_ACQ            0x30  /* Admin Completion Queue Base */

/* Controller Configuration bits */
#define NVME_CC_ENABLE          (1 << 0)
#define NVME_CC_IOSQES_SHIFT    16
#define NVME_CC_IOCQES_SHIFT    20

/* Controller Status bits */
#define NVME_CSTS_RDY           (1 << 0)
#define NVME_CSTS_CFS           (1 << 1)

/**
 * @brief NVMe command structure (64 bytes)
 */
struct nvme_command {
    uint8_t opcode;
    uint8_t flags;
    uint16_t command_id;
    uint32_t nsid;                /* Namespace ID */
    uint64_t rsvd2;
    uint64_t metadata;
    uint64_t prp1;                /* Physical region page 1 */
    uint64_t prp2;                /* Physical region page 2 */
    uint32_t cdw10;
    uint32_t cdw11;
    uint32_t cdw12;
    uint32_t cdw13;
    uint32_t cdw14;
    uint32_t cdw15;
} __attribute__((packed));

/**
 * @brief NVMe completion structure (16 bytes)
 */
struct nvme_completion {
    uint32_t result;
    uint32_t rsvd;
    uint16_t sq_head;
    uint16_t sq_id;
    uint16_t command_id;
    uint16_t status;
} __attribute__((packed));

/**
 * @brief NVMe queue pair (submission + completion)
 * 
 * Uses C23 atomics for lock-free queue management
 */
struct nvme_queue {
    struct nvme_command *sq;      /* Submission queue */
    struct nvme_completion *cq;   /* Completion queue */
    _Atomic uint32_t sq_head;     /* Submission queue head */
    _Atomic uint32_t sq_tail;     /* Submission queue tail */
    _Atomic uint32_t cq_head;     /* Completion queue head */
    uint32_t cq_phase;            /* Completion queue phase */
    uint16_t qid;                 /* Queue ID */
    uint16_t depth;               /* Queue depth */
    volatile void *sq_doorbell;   /* Submission doorbell register (MMIO) */
    volatile void *cq_doorbell;   /* Completion doorbell register (MMIO) */
};

/**
 * @brief NVMe controller structure
 */
struct nvme_ctrl {
    volatile void *bar;           /* Base address register (MMIO) */
    struct nvme_queue admin_q;    /* Admin queue */
    struct nvme_queue *io_queues; /* I/O queues */
    uint32_t num_queues;          /* Number of I/O queues */
    uint32_t page_size;           /* Page size */
    uint64_t cap;                 /* Controller capabilities */
    uint32_t doorbell_stride;     /* Doorbell stride (in bytes) */
    uint32_t max_transfer_size;   /* Maximum transfer size */
};

/* Function prototypes */

/**
 * @brief Initialize NVMe controller
 * @param ctrl Controller structure to initialize
 * @param bar Base address register (MMIO)
 * @return 0 on success, negative error code on failure
 */
int nvme_init(struct nvme_ctrl *ctrl, volatile void *bar);

/**
 * @brief Submit NVMe command to queue (with polling)
 * @param q Queue to submit to
 * @param cmd Command to submit
 * @return 0 on success, negative error code on failure
 */
int nvme_submit_cmd(struct nvme_queue *q, struct nvme_command *cmd);

/**
 * @brief Poll completion queue for completion (no interrupts)
 * @param q Queue to poll
 * @param cpl Completion structure to fill
 * @return 0 on success, -EAGAIN if no completion, negative error code on failure
 */
int nvme_poll_cq(struct nvme_queue *q, struct nvme_completion *cpl);

/**
 * @brief Read data from NVMe device (zero-copy)
 * @param ctrl Controller
 * @param nsid Namespace ID
 * @param lba Logical block address
 * @param buf Buffer to read into (must be physically contiguous)
 * @param count Number of blocks to read
 * @return 0 on success, negative error code on failure
 */
int nvme_read(struct nvme_ctrl *ctrl, uint32_t nsid, uint64_t lba, void *buf, size_t count);

/**
 * @brief Write data to NVMe device (zero-copy)
 * @param ctrl Controller
 * @param nsid Namespace ID
 * @param lba Logical block address
 * @param buf Buffer to write from (must be physically contiguous)
 * @param count Number of blocks to write
 * @return 0 on success, negative error code on failure
 */
int nvme_write(struct nvme_ctrl *ctrl, uint32_t nsid, uint64_t lba, const void *buf, size_t count);

/**
 * @brief Submit batch of commands (I/O batching)
 * @param q Queue to submit to
 * @param cmds Array of commands
 * @param count Number of commands
 * @return 0 on success, negative error code on failure
 */
int nvme_submit_batch(struct nvme_queue *q, struct nvme_command *cmds, size_t count);

/**
 * @brief Cleanup NVMe controller
 * @param ctrl Controller to cleanup
 */
void nvme_cleanup(struct nvme_ctrl *ctrl);

/* MMIO helper functions using C23 atomics */

/**
 * @brief Read 32-bit register with acquire semantics
 * @param addr Register address
 * @return Register value
 */
static inline uint32_t nvme_read_reg32(volatile void *addr) {
    return atomic_load_explicit((_Atomic uint32_t *)addr, memory_order_acquire);
}

/**
 * @brief Write 32-bit register with release semantics
 * @param addr Register address
 * @param val Value to write
 */
static inline void nvme_write_reg32(volatile void *addr, uint32_t val) {
    atomic_store_explicit((_Atomic uint32_t *)addr, val, memory_order_release);
}

/**
 * @brief Read 64-bit register with acquire semantics
 * @param addr Register address
 * @return Register value
 */
static inline uint64_t nvme_read_reg64(volatile void *addr) {
    return atomic_load_explicit((_Atomic uint64_t *)addr, memory_order_acquire);
}

/**
 * @brief Write 64-bit register with release semantics
 * @param addr Register address
 * @param val Value to write
 */
static inline void nvme_write_reg64(volatile void *addr, uint64_t val) {
    atomic_store_explicit((_Atomic uint64_t *)addr, val, memory_order_release);
}

/**
 * @brief Type-safe register read using typeof (C23)
 */
#define NVME_READ_REG(addr) ({ \
    typeof(*addr) __val; \
    __val = atomic_load_explicit((_Atomic typeof(*addr) *)addr, memory_order_acquire); \
    __val; \
})

/**
 * @brief Type-safe register write using typeof (C23)
 */
#define NVME_WRITE_REG(addr, val) ({ \
    atomic_store_explicit((_Atomic typeof(*addr) *)addr, val, memory_order_release); \
})

#endif /* BDI_NVME_H */
