
// ===================================================================
// DESC: NVMe Admin Command implementation for BDI Kernel
//       Handles NVMe administrative commands and queue management
// ===================================================================

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// NVMe Admin Command Opcodes
#define NVME_ADMIN_DELETE_SQ        0x00
#define NVME_ADMIN_CREATE_SQ        0x01
#define NVME_ADMIN_GET_LOG_PAGE     0x02
#define NVME_ADMIN_DELETE_CQ        0x04
#define NVME_ADMIN_CREATE_CQ        0x05
#define NVME_ADMIN_IDENTIFY         0x06
#define NVME_ADMIN_ABORT            0x08
#define NVME_ADMIN_SET_FEATURES     0x09
#define NVME_ADMIN_GET_FEATURES     0x0A
#define NVME_ADMIN_ASYNC_EVENT      0x0C
#define NVME_ADMIN_NS_MGMT          0x0D
#define NVME_ADMIN_FW_COMMIT        0x10
#define NVME_ADMIN_FW_DOWNLOAD      0x11
#define NVME_ADMIN_DEV_SELF_TEST    0x14
#define NVME_ADMIN_NS_ATTACH        0x15
#define NVME_ADMIN_KEEP_ALIVE       0x18
#define NVME_ADMIN_DIRECTIVE_SEND   0x19
#define NVME_ADMIN_DIRECTIVE_RECV   0x1A
#define NVME_ADMIN_SANITIZE         0x84

// NVMe Identify CNS Values
#define NVME_ID_CNS_NS              0x00
#define NVME_ID_CNS_CTRL            0x01
#define NVME_ID_CNS_NS_ACTIVE_LIST  0x02
#define NVME_ID_CNS_NS_DESC_LIST    0x03

// NVMe Feature IDs
#define NVME_FEAT_ARBITRATION       0x01
#define NVME_FEAT_POWER_MGMT        0x02
#define NVME_FEAT_LBA_RANGE         0x03
#define NVME_FEAT_TEMP_THRESH       0x04
#define NVME_FEAT_ERR_RECOVERY      0x05
#define NVME_FEAT_VOLATILE_WC       0x06
#define NVME_FEAT_NUM_QUEUES        0x07
#define NVME_FEAT_IRQ_COALESCE      0x08
#define NVME_FEAT_IRQ_CONFIG        0x09
#define NVME_FEAT_WRITE_ATOMIC      0x0A
#define NVME_FEAT_ASYNC_EVENT       0x0B

// NVMe Command Structure
typedef struct {
    uint8_t opcode;         // Command opcode
    uint8_t flags;          // Command flags
    uint16_t command_id;    // Command identifier
    uint32_t nsid;          // Namespace identifier
    uint64_t rsvd2[2];      // Reserved
    uint64_t metadata;      // Metadata pointer
    uint64_t prp1;          // PRP entry 1
    uint64_t prp2;          // PRP entry 2
    uint32_t cdw10;         // Command dword 10
    uint32_t cdw11;         // Command dword 11
    uint32_t cdw12;         // Command dword 12
    uint32_t cdw13;         // Command dword 13
    uint32_t cdw14;         // Command dword 14
    uint32_t cdw15;         // Command dword 15
} nvme_command_t;

// NVMe Completion Entry
typedef struct {
    uint32_t result;        // Command-specific result
    uint32_t rsvd;          // Reserved
    uint16_t sq_head;       // Submission queue head pointer
    uint16_t sq_id;         // Submission queue identifier
    uint16_t command_id;    // Command identifier
    uint16_t status;        // Status field
} nvme_completion_t;

// NVMe Controller Identify Data
typedef struct {
    uint16_t vid;           // Vendor ID
    uint16_t ssvid;         // Subsystem vendor ID
    char sn[20];            // Serial number
    char mn[40];            // Model number
    char fr[8];             // Firmware revision
    uint8_t rab;            // Recommended arbitration burst
    uint8_t ieee[3];        // IEEE OUI identifier
    uint8_t cmic;           // Controller multi-path I/O capabilities
    uint8_t mdts;           // Maximum data transfer size
    uint16_t cntlid;        // Controller ID
    uint32_t ver;           // Version
    uint32_t rtd3r;         // RTD3 resume latency
    uint32_t rtd3e;         // RTD3 entry latency
    uint32_t oaes;          // Optional async events supported
    uint32_t ctratt;        // Controller attributes
    uint8_t rsvd100[156];   // Reserved
    uint16_t oacs;          // Optional admin command support
    uint8_t acl;            // Abort command limit
    uint8_t aerl;           // Async event request limit
    uint8_t frmw;           // Firmware updates
    uint8_t lpa;            // Log page attributes
    uint8_t elpe;           // Error log page entries
    uint8_t npss;           // Number of power states support
    uint8_t avscc;          // Admin vendor specific command config
    uint8_t apsta;          // Autonomous power state transition attributes
    uint16_t wctemp;        // Warning composite temperature threshold
    uint16_t cctemp;        // Critical composite temperature threshold
    uint16_t mtfa;          // Maximum time for firmware activation
    uint32_t hmpre;         // Host memory buffer preferred size
    uint32_t hmmin;         // Host memory buffer minimum size
    uint8_t tnvmcap[16];    // Total NVM capacity
    uint8_t unvmcap[16];    // Unallocated NVM capacity
    uint32_t rpmbs;         // Replay protected memory block support
    uint16_t edstt;         // Extended device self-test time
    uint8_t dsto;           // Device self-test options
    uint8_t fwug;           // Firmware update granularity
    uint16_t kas;           // Keep alive support
    uint16_t hctma;         // Host controlled thermal management attributes
    uint16_t mntmt;         // Minimum thermal management temperature
    uint16_t mxtmt;         // Maximum thermal management temperature
    uint32_t sanicap;       // Sanitize capabilities
    uint8_t rsvd332[180];   // Reserved
    uint8_t sqes;           // Submission queue entry size
    uint8_t cqes;           // Completion queue entry size
    uint16_t maxcmd;        // Maximum outstanding commands
    uint32_t nn;            // Number of namespaces
    uint16_t oncs;          // Optional NVM command support
    uint16_t fuses;         // Fused operation support
    uint8_t fna;            // Format NVM attributes
    uint8_t vwc;            // Volatile write cache
    uint16_t awun;          // Atomic write unit normal
    uint16_t awupf;         // Atomic write unit power fail
    uint8_t nvscc;          // NVM vendor specific command config
    uint8_t rsvd531;        // Reserved
    uint16_t acwu;          // Atomic compare & write unit
    uint16_t rsvd534;       // Reserved
    uint32_t sgls;          // SGL support
    uint8_t rsvd540[228];   // Reserved
    char subnqn[256];       // Subsystem NVM qualified name
    uint8_t rsvd1024[768];  // Reserved
    uint8_t nvmsr[256];     // NVM subsystem report
    uint8_t vspd[256];      // Vendor specific
    uint8_t vs[1024];       // Vendor specific
} nvme_id_ctrl_t;

// Global admin queue state
static nvme_command_t *g_admin_sq = NULL;
static nvme_completion_t *g_admin_cq = NULL;
static uint16_t g_admin_sq_tail = 0;
static uint16_t g_admin_cq_head = 0;
static uint16_t g_command_id = 1;
static uint8_t g_admin_initialized = 0;

// Function prototypes
int nvme_admin_init(void *admin_sq, void *admin_cq, uint16_t queue_size);
int nvme_admin_submit_command(nvme_command_t *cmd);
int nvme_admin_wait_completion(uint16_t command_id, nvme_completion_t *cpl);
int nvme_admin_identify_controller(nvme_id_ctrl_t *ctrl_data);
int nvme_admin_identify_namespace(uint32_t nsid, void *ns_data);
int nvme_admin_create_io_cq(uint16_t qid, uint16_t qsize, uint64_t prp1, uint16_t iv);
int nvme_admin_create_io_sq(uint16_t qid, uint16_t qsize, uint64_t prp1, uint16_t cqid);
int nvme_admin_delete_io_sq(uint16_t qid);
int nvme_admin_delete_io_cq(uint16_t qid);
int nvme_admin_set_features(uint8_t fid, uint32_t dw11, uint32_t *result);
int nvme_admin_get_features(uint8_t fid, uint32_t *result);
int nvme_admin_get_log_page(uint8_t lid, uint32_t nsid, void *buffer, uint32_t len);
int nvme_admin_abort_command(uint16_t sqid, uint16_t cid);
void nvme_admin_cleanup(void);

/**
 * Initialize NVMe admin queues
 */
int nvme_admin_init(void *admin_sq, void *admin_cq, uint16_t queue_size) {
    if (!admin_sq || !admin_cq || queue_size == 0) {
        return -1;
    }
    
    g_admin_sq = (nvme_command_t *)admin_sq;
    g_admin_cq = (nvme_completion_t *)admin_cq;
    g_admin_sq_tail = 0;
    g_admin_cq_head = 0;
    g_command_id = 1;
    g_admin_initialized = 1;
    
    return 0;
}

/**
 * Submit admin command
 */
int nvme_admin_submit_command(nvme_command_t *cmd) {
    if (!g_admin_initialized || !cmd) {
        return -1;
    }
    
    // Assign command ID
    cmd->command_id = g_command_id++;
    if (g_command_id == 0) {
        g_command_id = 1; // Skip 0
    }
    
    // Copy command to submission queue
    memcpy(&g_admin_sq[g_admin_sq_tail], cmd, sizeof(nvme_command_t));
    
    // Update tail pointer
    g_admin_sq_tail = (g_admin_sq_tail + 1) % 64; // Assume 64 entries
    
    // In a real implementation, this would ring the doorbell
    // to notify the controller of the new command
    
    return cmd->command_id;
}

/**
 * Wait for command completion
 */
int nvme_admin_wait_completion(uint16_t command_id, nvme_completion_t *cpl) {
    if (!g_admin_initialized || !cpl) {
        return -1;
    }
    
    // In a real implementation, this would:
    // 1. Wait for interrupt or poll completion queue
    // 2. Check phase bit to detect new completions
    // 3. Match command ID and return completion entry
    
    // For simulation, create a successful completion
    memset(cpl, 0, sizeof(nvme_completion_t));
    cpl->command_id = command_id;
    cpl->status = 0; // Success
    cpl->sq_head = g_admin_sq_tail;
    
    return 0;
}

/**
 * Identify Controller command
 */
int nvme_admin_identify_controller(nvme_id_ctrl_t *ctrl_data) {
    if (!ctrl_data) {
        return -1;
    }
    
    // Prepare identify command
    nvme_command_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = NVME_ADMIN_IDENTIFY;
    cmd.nsid = 0;
    cmd.prp1 = (uint64_t)ctrl_data;
    cmd.cdw10 = NVME_ID_CNS_CTRL;
    
    // Submit command
    int cmd_id = nvme_admin_submit_command(&cmd);
    if (cmd_id < 0) {
        return -1;
    }
    
    // Wait for completion
    nvme_completion_t cpl;
    if (nvme_admin_wait_completion(cmd_id, &cpl) != 0) {
        return -1;
    }
    
    // For simulation, fill in some controller data
    memset(ctrl_data, 0, sizeof(nvme_id_ctrl_t));
    ctrl_data->vid = 0x1234;
    strcpy(ctrl_data->mn, "BDI NVMe Controller");
    strcpy(ctrl_data->sn, "BDI001");
    strcpy(ctrl_data->fr, "1.0");
    ctrl_data->nn = 1; // One namespace
    ctrl_data->mdts = 5; // 2^5 * 4KB = 128KB max transfer
    ctrl_data->oacs = 0x0007; // Support format, security, firmware
    
    return (cpl.status & 0x7FF) == 0 ? 0 : -1;
}

/**
 * Identify Namespace command
 */
int nvme_admin_identify_namespace(uint32_t nsid, void *ns_data) {
    if (!ns_data || nsid == 0) {
        return -1;
    }
    
    // Prepare identify command
    nvme_command_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = NVME_ADMIN_IDENTIFY;
    cmd.nsid = nsid;
    cmd.prp1 = (uint64_t)ns_data;
    cmd.cdw10 = NVME_ID_CNS_NS;
    
    // Submit command
    int cmd_id = nvme_admin_submit_command(&cmd);
    if (cmd_id < 0) {
        return -1;
    }
    
    // Wait for completion
    nvme_completion_t cpl;
    if (nvme_admin_wait_completion(cmd_id, &cpl) != 0) {
        return -1;
    }
    
    return (cpl.status & 0x7FF) == 0 ? 0 : -1;
}

/**
 * Create I/O Completion Queue
 */
int nvme_admin_create_io_cq(uint16_t qid, uint16_t qsize, uint64_t prp1, uint16_t iv) {
    if (qid == 0 || qsize == 0) {
        return -1;
    }
    
    // Prepare create CQ command
    nvme_command_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = NVME_ADMIN_CREATE_CQ;
    cmd.prp1 = prp1;
    cmd.cdw10 = ((uint32_t)qsize - 1) | ((uint32_t)qid << 16);
    cmd.cdw11 = 0x1 | ((uint32_t)iv << 16); // Physically contiguous, interrupt vector
    
    // Submit command
    int cmd_id = nvme_admin_submit_command(&cmd);
    if (cmd_id < 0) {
        return -1;
    }
    
    // Wait for completion
    nvme_completion_t cpl;
    if (nvme_admin_wait_completion(cmd_id, &cpl) != 0) {
        return -1;
    }
    
    return (cpl.status & 0x7FF) == 0 ? 0 : -1;
}

/**
 * Create I/O Submission Queue
 */
int nvme_admin_create_io_sq(uint16_t qid, uint16_t qsize, uint64_t prp1, uint16_t cqid) {
    if (qid == 0 || qsize == 0 || cqid == 0) {
        return -1;
    }
    
    // Prepare create SQ command
    nvme_command_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = NVME_ADMIN_CREATE_SQ;
    cmd.prp1 = prp1;
    cmd.cdw10 = ((uint32_t)qsize - 1) | ((uint32_t)qid << 16);
    cmd.cdw11 = 0x1 | ((uint32_t)cqid << 16); // Physically contiguous, CQ ID
    
    // Submit command
    int cmd_id = nvme_admin_submit_command(&cmd);
    if (cmd_id < 0) {
        return -1;
    }
    
    // Wait for completion
    nvme_completion_t cpl;
    if (nvme_admin_wait_completion(cmd_id, &cpl) != 0) {
        return -1;
    }
    
    return (cpl.status & 0x7FF) == 0 ? 0 : -1;
}

/**
 * Delete I/O Submission Queue
 */
int nvme_admin_delete_io_sq(uint16_t qid) {
    if (qid == 0) {
        return -1;
    }
    
    // Prepare delete SQ command
    nvme_command_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = NVME_ADMIN_DELETE_SQ;
    cmd.cdw10 = qid;
    
    // Submit command
    int cmd_id = nvme_admin_submit_command(&cmd);
    if (cmd_id < 0) {
        return -1;
    }
    
    // Wait for completion
    nvme_completion_t cpl;
    if (nvme_admin_wait_completion(cmd_id, &cpl) != 0) {
        return -1;
    }
    
    return (cpl.status & 0x7FF) == 0 ? 0 : -1;
}

/**
 * Delete I/O Completion Queue
 */
int nvme_admin_delete_io_cq(uint16_t qid) {
    if (qid == 0) {
        return -1;
    }
    
    // Prepare delete CQ command
    nvme_command_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = NVME_ADMIN_DELETE_CQ;
    cmd.cdw10 = qid;
    
    // Submit command
    int cmd_id = nvme_admin_submit_command(&cmd);
    if (cmd_id < 0) {
        return -1;
    }
    
    // Wait for completion
    nvme_completion_t cpl;
    if (nvme_admin_wait_completion(cmd_id, &cpl) != 0) {
        return -1;
    }
    
    return (cpl.status & 0x7FF) == 0 ? 0 : -1;
}

/**
 * Set Features command
 */
int nvme_admin_set_features(uint8_t fid, uint32_t dw11, uint32_t *result) {
    // Prepare set features command
    nvme_command_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = NVME_ADMIN_SET_FEATURES;
    cmd.cdw10 = fid;
    cmd.cdw11 = dw11;
    
    // Submit command
    int cmd_id = nvme_admin_submit_command(&cmd);
    if (cmd_id < 0) {
        return -1;
    }
    
    // Wait for completion
    nvme_completion_t cpl;
    if (nvme_admin_wait_completion(cmd_id, &cpl) != 0) {
        return -1;
    }
    
    if (result) {
        *result = cpl.result;
    }
    
    return (cpl.status & 0x7FF) == 0 ? 0 : -1;
}

/**
 * Get Features command
 */
int nvme_admin_get_features(uint8_t fid, uint32_t *result) {
    // Prepare get features command
    nvme_command_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = NVME_ADMIN_GET_FEATURES;
    cmd.cdw10 = fid;
    
    // Submit command
    int cmd_id = nvme_admin_submit_command(&cmd);
    if (cmd_id < 0) {
        return -1;
    }
    
    // Wait for completion
    nvme_completion_t cpl;
    if (nvme_admin_wait_completion(cmd_id, &cpl) != 0) {
        return -1;
    }
    
    if (result) {
        *result = cpl.result;
    }
    
    return (cpl.status & 0x7FF) == 0 ? 0 : -1;
}

/**
 * Get Log Page command
 */
int nvme_admin_get_log_page(uint8_t lid, uint32_t nsid, void *buffer, uint32_t len) {
    if (!buffer || len == 0) {
        return -1;
    }
    
    // Prepare get log page command
    nvme_command_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = NVME_ADMIN_GET_LOG_PAGE;
    cmd.nsid = nsid;
    cmd.prp1 = (uint64_t)buffer;
    cmd.cdw10 = lid | (((len / 4) - 1) << 16); // Length in dwords
    
    // Submit command
    int cmd_id = nvme_admin_submit_command(&cmd);
    if (cmd_id < 0) {
        return -1;
    }
    
    // Wait for completion
    nvme_completion_t cpl;
    if (nvme_admin_wait_completion(cmd_id, &cpl) != 0) {
        return -1;
    }
    
    return (cpl.status & 0x7FF) == 0 ? 0 : -1;
}

/**
 * Abort command
 */
int nvme_admin_abort_command(uint16_t sqid, uint16_t cid) {
    // Prepare abort command
    nvme_command_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = NVME_ADMIN_ABORT;
    cmd.cdw10 = cid | ((uint32_t)sqid << 16);
    
    // Submit command
    int cmd_id = nvme_admin_submit_command(&cmd);
    if (cmd_id < 0) {
        return -1;
    }
    
    // Wait for completion
    nvme_completion_t cpl;
    if (nvme_admin_wait_completion(cmd_id, &cpl) != 0) {
        return -1;
    }
    
    return (cpl.status & 0x7FF) == 0 ? 0 : -1;
}

/**
 * Cleanup admin queues
 */
void nvme_admin_cleanup(void) {
    g_admin_sq = NULL;
    g_admin_cq = NULL;
    g_admin_sq_tail = 0;
    g_admin_cq_head = 0;
    g_command_id = 1;
    g_admin_initialized = 0;
}
