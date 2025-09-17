
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
