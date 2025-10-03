
// ===================================================================
// DESC: AHCI SATA driver header - Advanced Host Controller Interface
//       Supporting SATA storage with command queuing and DMA
// ===================================================================
#ifndef AEON_AHCI_H
#define AEON_AHCI_H

#include <stdint.h>
#include <stdbool.h>

// --- AHCI Register Offsets ---
#define AHCI_GHC_CAP        0x00    // Host Capabilities
#define AHCI_GHC_GHC        0x04    // Global Host Control
#define AHCI_GHC_IS         0x08    // Interrupt Status
#define AHCI_GHC_PI         0x0C    // Ports Implemented
#define AHCI_GHC_VS         0x10    // Version
#define AHCI_GHC_CCC_CTL    0x14    // Command Completion Coalescing Control
#define AHCI_GHC_CCC_PORTS  0x18    // Command Completion Coalescing Ports
#define AHCI_GHC_EM_LOC     0x1C    // Enclosure Management Location
#define AHCI_GHC_EM_CTL     0x20    // Enclosure Management Control
#define AHCI_GHC_CAP2       0x24    // Host Capabilities Extended
#define AHCI_GHC_BOHC       0x28    // BIOS/OS Handoff Control and Status

// --- Port Register Offsets (Base: 0x100 + port * 0x80) ---
#define AHCI_PORT_CLB       0x00    // Command List Base Address
#define AHCI_PORT_CLBU      0x04    // Command List Base Address Upper
#define AHCI_PORT_FB        0x08    // FIS Base Address
#define AHCI_PORT_FBU       0x0C    // FIS Base Address Upper
#define AHCI_PORT_IS        0x10    // Interrupt Status
#define AHCI_PORT_IE        0x14    // Interrupt Enable
#define AHCI_PORT_CMD       0x18    // Command and Status
#define AHCI_PORT_TFD       0x20    // Task File Data
#define AHCI_PORT_SIG       0x24    // Signature
#define AHCI_PORT_SSTS      0x28    // Serial ATA Status
#define AHCI_PORT_SCTL      0x2C    // Serial ATA Control
#define AHCI_PORT_SERR      0x30    // Serial ATA Error
#define AHCI_PORT_SACT      0x34    // Serial ATA Active
#define AHCI_PORT_CI        0x38    // Command Issue
#define AHCI_PORT_SNTF      0x3C    // Serial ATA Notification

// --- Global Host Control Register Bits ---
#define AHCI_GHC_HR         (1 << 0)    // HBA Reset
#define AHCI_GHC_IE         (1 << 1)    // Interrupt Enable
#define AHCI_GHC_MRSM       (1 << 2)    // MSI Revert to Single Message
#define AHCI_GHC_AE         (1 << 31)   // AHCI Enable

// --- Port Command Register Bits ---
#define AHCI_PORT_CMD_ST    (1 << 0)    // Start
#define AHCI_PORT_CMD_SUD   (1 << 1)    // Spin-Up Device
#define AHCI_PORT_CMD_POD   (1 << 2)    // Power On Device
#define AHCI_PORT_CMD_CLO   (1 << 3)    // Command List Override
#define AHCI_PORT_CMD_FRE   (1 << 4)    // FIS Receive Enable
#define AHCI_PORT_CMD_CCS_MASK (0x1F << 8)  // Current Command Slot
#define AHCI_PORT_CMD_MPSS  (1 << 13)   // Mechanical Presence Switch State
#define AHCI_PORT_CMD_FR    (1 << 14)   // FIS Receive Running
#define AHCI_PORT_CMD_CR    (1 << 15)   // Command List Running
#define AHCI_PORT_CMD_CPS   (1 << 16)   // Cold Presence State
#define AHCI_PORT_CMD_PMA   (1 << 17)   // Port Multiplier Attached
#define AHCI_PORT_CMD_HPCP  (1 << 18)   // Hot Plug Capable Port
#define AHCI_PORT_CMD_MPSP  (1 << 19)   // Mechanical Presence Switch Attached
#define AHCI_PORT_CMD_CPD   (1 << 20)   // Cold Presence Detection
#define AHCI_PORT_CMD_ESP   (1 << 21)   // External SATA Port
#define AHCI_PORT_CMD_FBSCP (1 << 22)   // FIS-based Switching Capable Port
#define AHCI_PORT_CMD_APSTE (1 << 23)   // Automatic Partial to Slumber Transitions Enabled
#define AHCI_PORT_CMD_ATAPI (1 << 24)   // Device is ATAPI
#define AHCI_PORT_CMD_DLAE  (1 << 25)   // Drive LED on ATAPI Enable
#define AHCI_PORT_CMD_ALPE  (1 << 26)   // Aggressive Link Power Management Enable
#define AHCI_PORT_CMD_ASP   (1 << 27)   // Aggressive Slumber / Partial
#define AHCI_PORT_CMD_ICC_MASK (0xF << 28) // Interface Communication Control

// --- SATA Status Register Bits ---
#define AHCI_PORT_SSTS_DET_MASK  0xF     // Device Detection
#define AHCI_PORT_SSTS_SPD_MASK  (0xF << 4)  // Current Interface Speed
#define AHCI_PORT_SSTS_IPM_MASK  (0xF << 8)  // Interface Power Management

#define AHCI_PORT_SSTS_DET_NONE  0x0     // No device detected
#define AHCI_PORT_SSTS_DET_PRESENT 0x1   // Device detected but no PHY communication
#define AHCI_PORT_SSTS_DET_ESTABLISHED 0x3 // Device detected and PHY communication established

// --- Command List Entry (32 bytes) ---
// ===================================================================
// C23 Modernization - Constexpr Constants
// ===================================================================

// AHCI Queue Sizes
static const uint32_t AHCI_MAX_PORTS = 32;
static const uint32_t AHCI_MAX_CMDS = 32;
static const uint32_t AHCI_CMD_SLOT_SIZE = 32;

// AHCI Timeouts
static const uint32_t AHCI_CMD_TIMEOUT_MS = 5000;
static const uint32_t AHCI_PORT_RESET_TIMEOUT_MS = 1000;

// AHCI Transfer Sizes
static const uint32_t AHCI_MAX_TRANSFER_SIZE = (1024 * 1024);
static const uint32_t AHCI_ZERO_COPY_THRESHOLD = (64 * 1024);

// Cache Line Size
static const uint32_t CACHE_LINE_SIZE = 64;

typedef struct {
    uint8_t  cfl:5;         // Command FIS Length
    uint8_t  a:1;           // ATAPI
    uint8_t  w:1;           // Write
    uint8_t  p:1;           // Prefetchable
    uint8_t  r:1;           // Reset
    uint8_t  b:1;           // BIST
    uint8_t  c:1;           // Clear Busy upon R_OK
    uint8_t  reserved1:1;
    uint8_t  pmp:4;         // Port Multiplier Port
    uint16_t prdtl;         // Physical Region Descriptor Table Length
    uint32_t prdbc;         // Physical Region Descriptor Byte Count
    uint64_t ctba;          // Command Table Descriptor Base Address
    uint32_t reserved2[4];
} __attribute__((packed)) ahci_cmd_header_t;

// --- Physical Region Descriptor (16 bytes) ---
// ===================================================================
// C23 Modernization - Constexpr Constants
// ===================================================================

// AHCI Queue Sizes
static const uint32_t AHCI_MAX_PORTS = 32;
static const uint32_t AHCI_MAX_CMDS = 32;
static const uint32_t AHCI_CMD_SLOT_SIZE = 32;

// AHCI Timeouts
static const uint32_t AHCI_CMD_TIMEOUT_MS = 5000;
static const uint32_t AHCI_PORT_RESET_TIMEOUT_MS = 1000;

// AHCI Transfer Sizes
static const uint32_t AHCI_MAX_TRANSFER_SIZE = (1024 * 1024);
static const uint32_t AHCI_ZERO_COPY_THRESHOLD = (64 * 1024);

// Cache Line Size
static const uint32_t CACHE_LINE_SIZE = 64;

typedef struct {
    uint64_t dba;           // Data Base Address
    uint32_t reserved;
    uint32_t dbc:22;        // Data Byte Count
    uint32_t reserved2:9;
    uint32_t i:1;           // Interrupt on Completion
} __attribute__((packed)) ahci_prd_t;

// --- Command Table (up to 256 PRDs) ---
// ===================================================================
// C23 Modernization - Constexpr Constants
// ===================================================================

// AHCI Queue Sizes
static const uint32_t AHCI_MAX_PORTS = 32;
static const uint32_t AHCI_MAX_CMDS = 32;
static const uint32_t AHCI_CMD_SLOT_SIZE = 32;

// AHCI Timeouts
static const uint32_t AHCI_CMD_TIMEOUT_MS = 5000;
static const uint32_t AHCI_PORT_RESET_TIMEOUT_MS = 1000;

// AHCI Transfer Sizes
static const uint32_t AHCI_MAX_TRANSFER_SIZE = (1024 * 1024);
static const uint32_t AHCI_ZERO_COPY_THRESHOLD = (64 * 1024);

// Cache Line Size
static const uint32_t CACHE_LINE_SIZE = 64;

typedef struct {
    uint8_t cfis[64];       // Command FIS
    uint8_t acmd[16];       // ATAPI Command
    uint8_t reserved[48];
    ahci_prd_t prdt[1];     // Physical Region Descriptor Table (variable length)
} __attribute__((packed)) ahci_cmd_table_t;

// --- FIS Types ---
#define FIS_TYPE_REG_H2D    0x27    // Register FIS - Host to Device
#define FIS_TYPE_REG_D2H    0x34    // Register FIS - Device to Host
#define FIS_TYPE_DMA_ACT    0x39    // DMA Activate FIS - Device to Host
#define FIS_TYPE_DMA_SETUP  0x41    // DMA Setup FIS - Bidirectional
#define FIS_TYPE_DATA       0x46    // Data FIS - Bidirectional
#define FIS_TYPE_BIST       0x58    // BIST Activate FIS - Bidirectional
#define FIS_TYPE_PIO_SETUP  0x5F    // PIO Setup FIS - Device to Host
#define FIS_TYPE_DEV_BITS   0xA1    // Set Device Bits FIS - Device to Host

// --- Register FIS - Host to Device ---
// ===================================================================
// C23 Modernization - Constexpr Constants
// ===================================================================

// AHCI Queue Sizes
static const uint32_t AHCI_MAX_PORTS = 32;
static const uint32_t AHCI_MAX_CMDS = 32;
static const uint32_t AHCI_CMD_SLOT_SIZE = 32;

// AHCI Timeouts
static const uint32_t AHCI_CMD_TIMEOUT_MS = 5000;
static const uint32_t AHCI_PORT_RESET_TIMEOUT_MS = 1000;

// AHCI Transfer Sizes
static const uint32_t AHCI_MAX_TRANSFER_SIZE = (1024 * 1024);
static const uint32_t AHCI_ZERO_COPY_THRESHOLD = (64 * 1024);

// Cache Line Size
static const uint32_t CACHE_LINE_SIZE = 64;

typedef struct {
    uint8_t fis_type;       // FIS_TYPE_REG_H2D
    uint8_t pmport:4;       // Port multiplier
    uint8_t reserved1:3;
    uint8_t c:1;            // 1: Command, 0: Control
    uint8_t command;        // Command register
    uint8_t featurel;       // Feature register, 7:0
    uint8_t lba0;           // LBA low register, 7:0
    uint8_t lba1;           // LBA mid register, 15:8
    uint8_t lba2;           // LBA high register, 23:16
    uint8_t device;         // Device register
    uint8_t lba3;           // LBA register, 31:24
    uint8_t lba4;           // LBA register, 39:32
    uint8_t lba5;           // LBA register, 47:40
    uint8_t featureh;       // Feature register, 15:8
    uint8_t countl;         // Count register, 7:0
    uint8_t counth;         // Count register, 15:8
    uint8_t icc;            // Isochronous command completion
    uint8_t control;        // Control register
    uint8_t reserved2[4];
} __attribute__((packed)) fis_reg_h2d_t;

// --- ATA Commands ---
#define ATA_CMD_READ_DMA_EXT    0x25
#define ATA_CMD_WRITE_DMA_EXT   0x35
#define ATA_CMD_IDENTIFY        0xEC
#define ATA_CMD_FLUSH_CACHE_EXT 0xEA

// --- Port Structure ---
// ===================================================================
// C23 Modernization - Constexpr Constants
// ===================================================================

// AHCI Queue Sizes
static const uint32_t AHCI_MAX_PORTS = 32;
static const uint32_t AHCI_MAX_CMDS = 32;
static const uint32_t AHCI_CMD_SLOT_SIZE = 32;

// AHCI Timeouts
static const uint32_t AHCI_CMD_TIMEOUT_MS = 5000;
static const uint32_t AHCI_PORT_RESET_TIMEOUT_MS = 1000;

// AHCI Transfer Sizes
static const uint32_t AHCI_MAX_TRANSFER_SIZE = (1024 * 1024);
static const uint32_t AHCI_ZERO_COPY_THRESHOLD = (64 * 1024);

// Cache Line Size
static const uint32_t CACHE_LINE_SIZE = 64;

// Physical Region Descriptor Table Entry
#define AHCI_MAX_PRDT_ENTRIES 168  // Maximum PRDT entries per command

typedef struct {
    uint32_t dba;      // Data Base Address (lower 32 bits)
    uint32_t dbau;     // Data Base Address Upper (upper 32 bits)
    uint32_t reserved;
    uint32_t dbc;      // Data Byte Count (bits 0-21), I flag (bit 31)
} __attribute__((packed)) ahci_prdt_entry_t;

// Verify AHCI PRDT entry is exactly 16 bytes per hardware spec
_Static_assert(sizeof(ahci_prdt_entry_t) == 16, "AHCI PRDT entry must be 16 bytes");

typedef struct {
    uint32_t port_num;
    volatile uint8_t* port_base;
    ahci_cmd_header_t* cmd_list;
    uint8_t* fis_base;
    ahci_cmd_table_t* cmd_tables[32];
    bool device_present;
    uint64_t sectors;
    uint32_t sector_size;
    
    // Lock-free operation support
    _Atomic uint32_t cmd_slots_used;   // Bitmap of used command slots
    _Atomic uint32_t cmd_issue;        // Command issue register
    _Atomic uint32_t sata_active;      // SATA active register
    uint32_t max_slots;                // Maximum command slots (typically 32)
} ahci_port_t;

// --- Controller Structure ---
// ===================================================================
// C23 Modernization - Constexpr Constants
// ===================================================================

// AHCI Queue Sizes
static const uint32_t AHCI_MAX_PORTS = 32;
static const uint32_t AHCI_MAX_CMDS = 32;
static const uint32_t AHCI_CMD_SLOT_SIZE = 32;

// AHCI Timeouts
static const uint32_t AHCI_CMD_TIMEOUT_MS = 5000;
static const uint32_t AHCI_PORT_RESET_TIMEOUT_MS = 1000;

// AHCI Transfer Sizes
static const uint32_t AHCI_MAX_TRANSFER_SIZE = (1024 * 1024);
static const uint32_t AHCI_ZERO_COPY_THRESHOLD = (64 * 1024);

// Cache Line Size
static const uint32_t CACHE_LINE_SIZE = 64;

typedef struct {
    volatile uint8_t* mmio_base;
    uint32_t ports_implemented;
    uint32_t num_command_slots;
    ahci_port_t ports[32];
    bool initialized;
} ahci_controller_t;

// --- Function Declarations ---

// Controller management
[[nodiscard]] int ahci_init_controller(ahci_controller_t* ctrl, volatile uint8_t* mmio_base);
[[nodiscard]] int ahci_shutdown_controller(ahci_controller_t* ctrl);

// Port management
[[nodiscard]] int ahci_init_port(ahci_controller_t* ctrl, uint32_t port_num);
[[nodiscard]] int ahci_start_port(ahci_controller_t* ctrl, uint32_t port_num);
[[nodiscard]] int ahci_stop_port(ahci_controller_t* ctrl, uint32_t port_num);
[[nodiscard]] bool ahci_port_has_device(ahci_controller_t* ctrl, uint32_t port_num);

// Command operations
[[nodiscard]] int ahci_identify_device(ahci_controller_t* ctrl, uint32_t port_num, void* buffer);
[[nodiscard]] int ahci_read_sectors(ahci_controller_t* ctrl, uint32_t port_num, uint64_t lba, uint32_t count, void* buffer);
[[nodiscard]] int ahci_write_sectors(ahci_controller_t* ctrl, uint32_t port_num, uint64_t lba, uint32_t count, const void* buffer);
[[nodiscard]] int ahci_flush_cache(ahci_controller_t* ctrl, uint32_t port_num);

// Utility functions
[[nodiscard]] uint32_t ahci_read_reg32(ahci_controller_t* ctrl, uint32_t offset);
void ahci_write_reg32(ahci_controller_t* ctrl, uint32_t offset, uint32_t value);
[[nodiscard]] uint32_t ahci_port_read_reg32(ahci_controller_t* ctrl, uint32_t port_num, uint32_t offset);
void ahci_port_write_reg32(ahci_controller_t* ctrl, uint32_t port_num, uint32_t offset, uint32_t value);

// Error codes
#define AHCI_SUCCESS            0
#define AHCI_ERROR_TIMEOUT      -1
#define AHCI_ERROR_INVALID      -2
#define AHCI_ERROR_NO_MEMORY    -3
#define AHCI_ERROR_IO           -4
#define AHCI_ERROR_NOT_READY    -5

#endif // AEON_AHCI_H
