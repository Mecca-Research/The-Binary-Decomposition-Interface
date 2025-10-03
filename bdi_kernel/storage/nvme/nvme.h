
// ===================================================================
// DESC: NVMe driver header - Advanced NVMe controller implementation
//       Supporting admin/IO queues, doorbells, and completions
// ===================================================================
#ifndef AEON_NVME_H
#define AEON_NVME_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// --- NVMe Register Offsets ---
#define NVME_REG_CAP        0x00    // Controller Capabilities
#define NVME_REG_VS         0x08    // Version
#define NVME_REG_INTMS      0x0C    // Interrupt Mask Set
#define NVME_REG_INTMC      0x10    // Interrupt Mask Clear
#define NVME_REG_CC         0x14    // Controller Configuration
#define NVME_REG_CSTS       0x1C    // Controller Status
#define NVME_REG_NSSR       0x20    // NVM Subsystem Reset
#define NVME_REG_AQA        0x24    // Admin Queue Attributes
#define NVME_REG_ASQ        0x28    // Admin Submission Queue Base
#define NVME_REG_ACQ        0x30    // Admin Completion Queue Base
#define NVME_REG_CMBLOC     0x38    // Controller Memory Buffer Location
#define NVME_REG_CMBSZ      0x3C    // Controller Memory Buffer Size

// --- Doorbell Registers (Base: 0x1000) ---
#define NVME_REG_DBS        0x1000  // Doorbell Base

// --- Controller Capabilities Register Bits ---
#define NVME_CAP_MQES_MASK  0xFFFF          // Maximum Queue Entries Supported
#define NVME_CAP_CQR        (1ULL << 16)    // Contiguous Queues Required
#define NVME_CAP_AMS_MASK   (3ULL << 17)    // Arbitration Mechanism Supported
#define NVME_CAP_TO_MASK    (0xFFULL << 24) // Timeout
#define NVME_CAP_DSTRD_MASK (0xFULL << 32)  // Doorbell Stride
#define NVME_CAP_NSSRS      (1ULL << 36)    // NVM Subsystem Reset Supported
#define NVME_CAP_CSS_MASK   (0xFFULL << 37) // Command Sets Supported
#define NVME_CAP_BPS        (1ULL << 45)    // Boot Partition Support
#define NVME_CAP_MPSMIN_MASK (0xFULL << 48) // Memory Page Size Minimum
#define NVME_CAP_MPSMAX_MASK (0xFULL << 52) // Memory Page Size Maximum
#define NVME_CAP_PMRS       (1ULL << 56)    // Persistent Memory Region Supported
#define NVME_CAP_CMBS       (1ULL << 57)    // Controller Memory Buffer Supported

// --- Controller Configuration Register Bits ---
#define NVME_CC_EN          (1 << 0)        // Enable
#define NVME_CC_CSS_MASK    (7 << 4)        // Command Set Selected
#define NVME_CC_MPS_MASK    (0xF << 7)      // Memory Page Size
#define NVME_CC_AMS_MASK    (7 << 11)       // Arbitration Mechanism Selected
#define NVME_CC_SHN_MASK    (3 << 14)       // Shutdown Notification
#define NVME_CC_IOSQES_MASK (0xF << 16)     // I/O Submission Queue Entry Size
#define NVME_CC_IOCQES_MASK (0xF << 20)     // I/O Completion Queue Entry Size

// --- Controller Status Register Bits ---
#define NVME_CSTS_RDY       (1 << 0)        // Ready
#define NVME_CSTS_CFS       (1 << 1)        // Controller Fatal Status
#define NVME_CSTS_SHST_MASK (3 << 2)        // Shutdown Status
#define NVME_CSTS_NSSRO     (1 << 4)        // NVM Subsystem Reset Occurred
#define NVME_CSTS_PP        (1 << 5)        // Processing Paused

// --- Queue Sizes ---
#define NVME_ADMIN_QUEUE_SIZE   64
#define NVME_IO_QUEUE_SIZE      256
#define NVME_MAX_QUEUES         16

// --- Command Opcodes ---
#define NVME_ADMIN_DELETE_SQ    0x00
#define NVME_ADMIN_CREATE_SQ    0x01
#define NVME_ADMIN_GET_LOG_PAGE 0x02
#define NVME_ADMIN_DELETE_CQ    0x04
#define NVME_ADMIN_CREATE_CQ    0x05
#define NVME_ADMIN_IDENTIFY     0x06
#define NVME_ADMIN_ABORT        0x08
#define NVME_ADMIN_SET_FEATURES 0x09
#define NVME_ADMIN_GET_FEATURES 0x0A

#define NVME_CMD_READ           0x02
#define NVME_CMD_WRITE          0x01
#define NVME_CMD_FLUSH          0x00

// --- Data Structures ---

// NVMe Command (64 bytes)
// ===================================================================
// C23 Modernization - Constexpr Constants
// ===================================================================

// NVMe Queue Sizes (const for compile-time constants)
static const uint32_t NVME_ADMIN_QUEUE_SIZE = 64;
static const uint32_t NVME_IO_QUEUE_SIZE = 1024;
static const uint32_t NVME_MAX_IO_QUEUES = 128;
static const uint32_t NVME_QUEUE_ALIGNMENT = 64;

// NVMe Timeouts (in milliseconds)
static const uint32_t NVME_ADMIN_TIMEOUT_MS = 5000;
static const uint32_t NVME_IO_TIMEOUT_MS = 30000;
static const uint32_t NVME_RESET_TIMEOUT_MS = 10000;

// NVMe Transfer Sizes
static const uint32_t NVME_MAX_TRANSFER_SIZE = (1024 * 1024);  // 1MB
static const uint32_t NVME_ZERO_COPY_THRESHOLD = (64 * 1024);  // 64KB

// Cache Line Size for Alignment
static const uint32_t CACHE_LINE_SIZE = 64;

typedef struct {
    uint8_t  opcode;        // Command opcode
    uint8_t  flags;         // Command flags
    uint16_t command_id;    // Command identifier
    uint32_t nsid;          // Namespace identifier
    uint64_t reserved1;
    uint64_t metadata;      // Metadata pointer
    uint64_t prp1;          // Physical Region Page 1
    uint64_t prp2;          // Physical Region Page 2
    uint32_t cdw10;         // Command-specific
    uint32_t cdw11;         // Command-specific
    uint32_t cdw12;         // Command-specific
    uint32_t cdw13;         // Command-specific
    uint32_t cdw14;         // Command-specific
    uint32_t cdw15;         // Command-specific
} __attribute__((packed)) nvme_command_t;

// NVMe Completion (16 bytes)
// ===================================================================
// C23 Modernization - Constexpr Constants
// ===================================================================

// NVMe Queue Sizes (const for compile-time constants)
static const uint32_t NVME_ADMIN_QUEUE_SIZE = 64;
static const uint32_t NVME_IO_QUEUE_SIZE = 1024;
static const uint32_t NVME_MAX_IO_QUEUES = 128;
static const uint32_t NVME_QUEUE_ALIGNMENT = 64;

// NVMe Timeouts (in milliseconds)
static const uint32_t NVME_ADMIN_TIMEOUT_MS = 5000;
static const uint32_t NVME_IO_TIMEOUT_MS = 30000;
static const uint32_t NVME_RESET_TIMEOUT_MS = 10000;

// NVMe Transfer Sizes
static const uint32_t NVME_MAX_TRANSFER_SIZE = (1024 * 1024);  // 1MB
static const uint32_t NVME_ZERO_COPY_THRESHOLD = (64 * 1024);  // 64KB

// Cache Line Size for Alignment
static const uint32_t CACHE_LINE_SIZE = 64;

typedef struct {
    uint32_t result;        // Command-specific result
    uint32_t reserved;
    uint16_t sq_head;       // Submission queue head pointer
    uint16_t sq_id;         // Submission queue identifier
    uint16_t command_id;    // Command identifier
    uint16_t status;        // Status field
} __attribute__((packed)) nvme_completion_t;
// ===================================================================
// C23 Modernization - Constexpr Constants
// ===================================================================

// NVMe Queue Sizes (const for compile-time constants)
static const uint32_t NVME_ADMIN_QUEUE_SIZE = 64;
static const uint32_t NVME_IO_QUEUE_SIZE = 1024;
static const uint32_t NVME_MAX_IO_QUEUES = 128;
static const uint32_t NVME_QUEUE_ALIGNMENT = 64;

// NVMe Timeouts (in milliseconds)
static const uint32_t NVME_ADMIN_TIMEOUT_MS = 5000;
static const uint32_t NVME_IO_TIMEOUT_MS = 30000;
static const uint32_t NVME_RESET_TIMEOUT_MS = 10000;

// NVMe Transfer Sizes
static const uint32_t NVME_MAX_TRANSFER_SIZE = (1024 * 1024);  // 1MB
static const uint32_t NVME_ZERO_COPY_THRESHOLD = (64 * 1024);  // 64KB

// Cache Line Size for Alignment
static const uint32_t CACHE_LINE_SIZE = 64;

_Static_assert(sizeof(typedef struct {
    uint32_t result;        // Command-specific result
    uint32_t reserved;
    uint16_t sq_head;       // Submission queue head pointer
    uint16_t sq_id;         // Submission queue identifier
    uint16_t command_id;    // Command identifier
    uint16_t status;        // Status field
// ===================================================================
// C23 Modernization - Constexpr Constants
// ===================================================================

// NVMe Queue Sizes (const for compile-time constants)
static const uint32_t NVME_ADMIN_QUEUE_SIZE = 64;
static const uint32_t NVME_IO_QUEUE_SIZE = 1024;
static const uint32_t NVME_MAX_IO_QUEUES = 128;
static const uint32_t NVME_QUEUE_ALIGNMENT = 64;

// NVMe Timeouts (in milliseconds)
static const uint32_t NVME_ADMIN_TIMEOUT_MS = 5000;
static const uint32_t NVME_IO_TIMEOUT_MS = 30000;
static const uint32_t NVME_RESET_TIMEOUT_MS = 10000;

// NVMe Transfer Sizes
static const uint32_t NVME_MAX_TRANSFER_SIZE = (1024 * 1024);  // 1MB
static const uint32_t NVME_ZERO_COPY_THRESHOLD = (64 * 1024);  // 64KB

// Cache Line Size for Alignment
static const uint32_t CACHE_LINE_SIZE = 64;

} __attribute__((packed)) nvme_completion_t;) % 64 == 0, "typedef struct {
    uint32_t result;        // Command-specific result
    uint32_t reserved;
    uint16_t sq_head;       // Submission queue head pointer
    uint16_t sq_id;         // Submission queue identifier
    uint16_t command_id;    // Command identifier
    uint16_t status;        // Status field
} __attribute__((packed)) nvme_completion_t; must be cache-aligned");

// Queue structures
// ===================================================================
// C23 Modernization - Constexpr Constants
// ===================================================================

// NVMe Queue Sizes (const for compile-time constants)
static const uint32_t NVME_ADMIN_QUEUE_SIZE = 64;
static const uint32_t NVME_IO_QUEUE_SIZE = 1024;
static const uint32_t NVME_MAX_IO_QUEUES = 128;
static const uint32_t NVME_QUEUE_ALIGNMENT = 64;

// NVMe Timeouts (in milliseconds)
static const uint32_t NVME_ADMIN_TIMEOUT_MS = 5000;
static const uint32_t NVME_IO_TIMEOUT_MS = 30000;
static const uint32_t NVME_RESET_TIMEOUT_MS = 10000;

// NVMe Transfer Sizes
static const uint32_t NVME_MAX_TRANSFER_SIZE = (1024 * 1024);  // 1MB
static const uint32_t NVME_ZERO_COPY_THRESHOLD = (64 * 1024);  // 64KB

// Cache Line Size for Alignment
static const uint32_t CACHE_LINE_SIZE = 64;

typedef struct {
    nvme_command_t* commands;
    _Atomic uint32_t head;
    _Atomic uint32_t tail;
    uint32_t size;
    uint16_t queue_id;
    volatile uint32_t* doorbell;
} nvme_sq_t;
_Static_assert(sizeof(nvme_sq_t) % 64 == 0, "nvme_sq_t must be cache-aligned");

// ===================================================================
// C23 Modernization - Constexpr Constants
// ===================================================================

// NVMe Queue Sizes (const for compile-time constants)
static const uint32_t NVME_ADMIN_QUEUE_SIZE = 64;
static const uint32_t NVME_IO_QUEUE_SIZE = 1024;
static const uint32_t NVME_MAX_IO_QUEUES = 128;
static const uint32_t NVME_QUEUE_ALIGNMENT = 64;

// NVMe Timeouts (in milliseconds)
static const uint32_t NVME_ADMIN_TIMEOUT_MS = 5000;
static const uint32_t NVME_IO_TIMEOUT_MS = 30000;
static const uint32_t NVME_RESET_TIMEOUT_MS = 10000;

// NVMe Transfer Sizes
static const uint32_t NVME_MAX_TRANSFER_SIZE = (1024 * 1024);  // 1MB
static const uint32_t NVME_ZERO_COPY_THRESHOLD = (64 * 1024);  // 64KB

// Cache Line Size for Alignment
static const uint32_t CACHE_LINE_SIZE = 64;

typedef struct {
    nvme_completion_t* completions;
    _Atomic uint32_t head;
    _Atomic uint32_t tail;
    uint32_t size;
    uint16_t queue_id;
    uint8_t phase;
    volatile uint32_t* doorbell;
} nvme_cq_t;
_Static_assert(sizeof(nvme_cq_t) % 64 == 0, "nvme_cq_t must be cache-aligned");

// Controller structure
// ===================================================================
// C23 Modernization - Constexpr Constants
// ===================================================================

// NVMe Queue Sizes (const for compile-time constants)
static const uint32_t NVME_ADMIN_QUEUE_SIZE = 64;
static const uint32_t NVME_IO_QUEUE_SIZE = 1024;
static const uint32_t NVME_MAX_IO_QUEUES = 128;
static const uint32_t NVME_QUEUE_ALIGNMENT = 64;

// NVMe Timeouts (in milliseconds)
static const uint32_t NVME_ADMIN_TIMEOUT_MS = 5000;
static const uint32_t NVME_IO_TIMEOUT_MS = 30000;
static const uint32_t NVME_RESET_TIMEOUT_MS = 10000;

// NVMe Transfer Sizes
static const uint32_t NVME_MAX_TRANSFER_SIZE = (1024 * 1024);  // 1MB
static const uint32_t NVME_ZERO_COPY_THRESHOLD = (64 * 1024);  // 64KB

// Cache Line Size for Alignment
static const uint32_t CACHE_LINE_SIZE = 64;

typedef struct {
    volatile uint8_t* mmio_base;
    uint64_t capabilities;
    uint32_t doorbell_stride;
    uint16_t max_queue_entries;
    
    // Admin queues
    nvme_sq_t admin_sq;
    nvme_cq_t admin_cq;
    
    // I/O queues
    nvme_sq_t io_sq[NVME_MAX_QUEUES];
    nvme_cq_t io_cq[NVME_MAX_QUEUES];
    uint16_t num_io_queues;
    
    // Namespace info
    uint32_t num_namespaces;
    uint64_t namespace_size[16];  // Size in blocks
    uint32_t block_size[16];      // Block size in bytes
    
    bool initialized;
} nvme_controller_t;

// --- Function Declarations ---

// Controller management
[[nodiscard]] int nvme_init_controller(nvme_controller_t* ctrl, volatile uint8_t* mmio_base);
[[nodiscard]] int nvme_shutdown_controller(nvme_controller_t* ctrl);
[[nodiscard]] int nvme_reset_controller(nvme_controller_t* ctrl);

// Queue management
[[nodiscard]] int nvme_setup_admin_queues(nvme_controller_t* ctrl);
[[nodiscard]] int nvme_create_io_queue_pair(nvme_controller_t* ctrl, uint16_t queue_id, uint16_t queue_size);
[[nodiscard]] int nvme_delete_io_queue_pair(nvme_controller_t* ctrl, uint16_t queue_id);

// Command submission and completion
[[nodiscard]] int nvme_submit_admin_command(nvme_controller_t* ctrl, nvme_command_t* cmd, nvme_completion_t* completion);
[[nodiscard]] int nvme_submit_io_command(nvme_controller_t* ctrl, uint16_t queue_id, nvme_command_t* cmd);
[[nodiscard]] int nvme_poll_completion(nvme_controller_t* ctrl, uint16_t queue_id, nvme_completion_t* completion);

// High-level operations
[[nodiscard]] int nvme_identify_controller(nvme_controller_t* ctrl, void* data);
[[nodiscard]] int nvme_identify_namespace(nvme_controller_t* ctrl, uint32_t nsid, void* data);
[[nodiscard]] int nvme_read_blocks(nvme_controller_t* ctrl, uint32_t nsid, uint64_t lba, uint32_t count, void* buffer);
[[nodiscard]] int nvme_write_blocks(nvme_controller_t* ctrl, uint32_t nsid, uint64_t lba, uint32_t count, const void* buffer);
[[nodiscard]] int nvme_flush(nvme_controller_t* ctrl, uint32_t nsid);

// Utility functions
[[nodiscard]] uint32_t nvme_read_reg32(nvme_controller_t* ctrl, uint32_t offset);
[[nodiscard]] uint64_t nvme_read_reg64(nvme_controller_t* ctrl, uint32_t offset);
void nvme_write_reg32(nvme_controller_t* ctrl, uint32_t offset, uint32_t value);
void nvme_write_reg64(nvme_controller_t* ctrl, uint32_t offset, uint64_t value);

// Error codes
#define NVME_SUCCESS            0
#define NVME_ERROR_TIMEOUT      -1
#define NVME_ERROR_INVALID      -2
#define NVME_ERROR_NO_MEMORY    -3
#define NVME_ERROR_IO           -4
#define NVME_ERROR_NOT_READY    -5

#endif // AEON_NVME_H
