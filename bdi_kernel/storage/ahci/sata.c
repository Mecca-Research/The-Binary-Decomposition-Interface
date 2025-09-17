
// ===================================================================
// DESC: SATA driver implementation for AHCI controller in BDI Kernel
//       Provides SATA device management and I/O operations
// ===================================================================

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// SATA Constants
#define SATA_MAX_DEVICES        32
#define SATA_SECTOR_SIZE        512
#define SATA_MAX_SECTORS        65536
#define SATA_TIMEOUT_MS         5000

// SATA Device Types
#define SATA_TYPE_UNKNOWN       0
#define SATA_TYPE_ATA           1
#define SATA_TYPE_ATAPI         2
#define SATA_TYPE_SEMB          3
#define SATA_TYPE_PM            4

// SATA Command Types
#define SATA_CMD_READ_DMA       0xC8
#define SATA_CMD_WRITE_DMA      0xCA
#define SATA_CMD_READ_DMA_EXT   0x25
#define SATA_CMD_WRITE_DMA_EXT  0x35
#define SATA_CMD_IDENTIFY       0xEC
#define SATA_CMD_IDENTIFY_PACKET 0xA1
#define SATA_CMD_FLUSH_CACHE    0xE7

// SATA Status Bits
#define SATA_STATUS_ERR         0x01
#define SATA_STATUS_DRQ         0x08
#define SATA_STATUS_SRV         0x10
#define SATA_STATUS_DF          0x20
#define SATA_STATUS_RDY         0x40
#define SATA_STATUS_BSY         0x80

// SATA Device Structure
typedef struct {
    uint32_t port;                      // AHCI port number
    uint32_t device_type;               // Device type
    uint64_t sectors;                   // Total sectors
    uint32_t sector_size;               // Sector size in bytes
    char model[41];                     // Device model string
    char serial[21];                    // Device serial number
    char firmware[9];                   // Firmware revision
    uint32_t features;                  // Supported features
    uint8_t active;                     // Device active flag
} sata_device_t;

// SATA Command Structure
typedef struct {
    uint8_t command;                    // ATA command
    uint8_t features;                   // Features register
    uint64_t lba;                       // Logical block address
    uint32_t count;                     // Sector count
    void *buffer;                       // Data buffer
    uint32_t buffer_size;               // Buffer size
    uint32_t flags;                     // Command flags
} sata_command_t;

// Global SATA state
static sata_device_t g_sata_devices[SATA_MAX_DEVICES];
static uint32_t g_device_count = 0;
static uint8_t g_initialized = 0;

// Function prototypes
int sata_init(void);
int sata_probe_devices(void);
int sata_identify_device(uint32_t port, sata_device_t *device);
int sata_read_sectors(uint32_t device_id, uint64_t lba, uint32_t count, void *buffer);
int sata_write_sectors(uint32_t device_id, uint64_t lba, uint32_t count, const void *buffer);
int sata_flush_cache(uint32_t device_id);
int sata_execute_command(uint32_t port, sata_command_t *cmd);
int sata_wait_ready(uint32_t port, uint32_t timeout_ms);
uint32_t sata_get_device_count(void);
sata_device_t *sata_get_device(uint32_t device_id);
void sata_cleanup(void);

/**
 * Initialize SATA subsystem
 */
int sata_init(void) {
    if (g_initialized) {
        return 0;
    }
    
    // Clear device array
    memset(g_sata_devices, 0, sizeof(g_sata_devices));
    g_device_count = 0;
    
    // Probe for SATA devices
    if (sata_probe_devices() != 0) {
        return -1;
    }
    
    g_initialized = 1;
    return 0;
}

/**
 * Probe for SATA devices on all ports
 */
int sata_probe_devices(void) {
    // In a real implementation, this would:
    // 1. Check AHCI controller for active ports
    // 2. For each active port, check device signature
    // 3. Identify each device and populate device structure
    
    // Simulate finding one SATA device
    sata_device_t *device = &g_sata_devices[0];
    device->port = 0;
    device->device_type = SATA_TYPE_ATA;
    device->sectors = 2097152; // 1GB in 512-byte sectors
    device->sector_size = SATA_SECTOR_SIZE;
    strcpy(device->model, "BDI Virtual SATA Drive");
    strcpy(device->serial, "BDI001");
    strcpy(device->firmware, "1.0");
    device->features = 0;
    device->active = 1;
    
    g_device_count = 1;
    return 0;
}

/**
 * Identify a SATA device
 */
int sata_identify_device(uint32_t port, sata_device_t *device) {
    if (!device) {
        return -1;
    }
    
    // Prepare IDENTIFY command
    sata_command_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.command = SATA_CMD_IDENTIFY;
    cmd.count = 1;
    
    // Allocate buffer for identify data
    uint16_t *identify_data = (uint16_t *)malloc(512);
    if (!identify_data) {
        return -1;
    }
    
    cmd.buffer = identify_data;
    cmd.buffer_size = 512;
    
    // Execute command
    int result = sata_execute_command(port, &cmd);
    if (result != 0) {
        free(identify_data);
        return -1;
    }
    
    // Parse identify data
    device->sectors = ((uint64_t)identify_data[103] << 48) |
                     ((uint64_t)identify_data[102] << 32) |
                     ((uint64_t)identify_data[101] << 16) |
                     identify_data[100];
    
    // Extract model string (words 27-46)
    for (int i = 0; i < 20; i++) {
        uint16_t word = identify_data[27 + i];
        device->model[i * 2] = (word >> 8) & 0xFF;
        device->model[i * 2 + 1] = word & 0xFF;
    }
    device->model[40] = '\0';
    
    // Extract serial number (words 10-19)
    for (int i = 0; i < 10; i++) {
        uint16_t word = identify_data[10 + i];
        device->serial[i * 2] = (word >> 8) & 0xFF;
        device->serial[i * 2 + 1] = word & 0xFF;
    }
    device->serial[20] = '\0';
    
    // Extract firmware revision (words 23-26)
    for (int i = 0; i < 4; i++) {
        uint16_t word = identify_data[23 + i];
        device->firmware[i * 2] = (word >> 8) & 0xFF;
        device->firmware[i * 2 + 1] = word & 0xFF;
    }
    device->firmware[8] = '\0';
    
    free(identify_data);
    return 0;
}

/**
 * Read sectors from SATA device
 */
int sata_read_sectors(uint32_t device_id, uint64_t lba, uint32_t count, void *buffer) {
    if (device_id >= g_device_count || !buffer || count == 0) {
        return -1;
    }
    
    sata_device_t *device = &g_sata_devices[device_id];
    if (!device->active) {
        return -1;
    }
    
    // Check bounds
    if (lba + count > device->sectors) {
        return -1;
    }
    
    // Prepare read command
    sata_command_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    
    if (lba + count <= 0x10000000ULL) {
        // Use 28-bit LBA command
        cmd.command = SATA_CMD_READ_DMA;
    } else {
        // Use 48-bit LBA command
        cmd.command = SATA_CMD_READ_DMA_EXT;
    }
    
    cmd.lba = lba;
    cmd.count = count;
    cmd.buffer = buffer;
    cmd.buffer_size = count * device->sector_size;
    
    // Execute command
    return sata_execute_command(device->port, &cmd);
}

/**
 * Write sectors to SATA device
 */
int sata_write_sectors(uint32_t device_id, uint64_t lba, uint32_t count, const void *buffer) {
    if (device_id >= g_device_count || !buffer || count == 0) {
        return -1;
    }
    
    sata_device_t *device = &g_sata_devices[device_id];
    if (!device->active) {
        return -1;
    }
    
    // Check bounds
    if (lba + count > device->sectors) {
        return -1;
    }
    
    // Prepare write command
    sata_command_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    
    if (lba + count <= 0x10000000ULL) {
        // Use 28-bit LBA command
        cmd.command = SATA_CMD_WRITE_DMA;
    } else {
        // Use 48-bit LBA command
        cmd.command = SATA_CMD_WRITE_DMA_EXT;
    }
    
    cmd.lba = lba;
    cmd.count = count;
    cmd.buffer = (void *)buffer;
    cmd.buffer_size = count * device->sector_size;
    
    // Execute command
    return sata_execute_command(device->port, &cmd);
}

/**
 * Flush cache for SATA device
 */
int sata_flush_cache(uint32_t device_id) {
    if (device_id >= g_device_count) {
        return -1;
    }
    
    sata_device_t *device = &g_sata_devices[device_id];
    if (!device->active) {
        return -1;
    }
    
    // Prepare flush command
    sata_command_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.command = SATA_CMD_FLUSH_CACHE;
    
    // Execute command
    return sata_execute_command(device->port, &cmd);
}

/**
 * Execute SATA command
 */
int sata_execute_command(uint32_t port, sata_command_t *cmd) {
    if (!cmd) {
        return -1;
    }
    
    // In a real implementation, this would:
    // 1. Build command FIS (Frame Information Structure)
    // 2. Set up PRD (Physical Region Descriptor) table for DMA
    // 3. Write command to AHCI command list
    // 4. Ring doorbell to start command
    // 5. Wait for completion interrupt
    // 6. Check status and handle errors
    
    // For simulation, just wait and return success
    if (sata_wait_ready(port, SATA_TIMEOUT_MS) != 0) {
        return -1;
    }
    
    // Simulate data transfer for read/write commands
    if (cmd->buffer && cmd->buffer_size > 0) {
        if (cmd->command == SATA_CMD_READ_DMA || cmd->command == SATA_CMD_READ_DMA_EXT) {
            // Simulate reading data (fill with pattern)
            memset(cmd->buffer, 0xAA, cmd->buffer_size);
        }
        // For write commands, data is already in buffer
    }
    
    return 0;
}

/**
 * Wait for device to become ready
 */
int sata_wait_ready(uint32_t port, uint32_t timeout_ms) {
    // In a real implementation, this would:
    // 1. Read AHCI port task file data register
    // 2. Check BSY and DRQ bits in status
    // 3. Wait with timeout for device to become ready
    
    // For simulation, just return success
    (void)port;
    (void)timeout_ms;
    return 0;
}

/**
 * Get number of detected SATA devices
 */
uint32_t sata_get_device_count(void) {
    return g_device_count;
}

/**
 * Get SATA device information
 */
sata_device_t *sata_get_device(uint32_t device_id) {
    if (device_id >= g_device_count) {
        return NULL;
    }
    
    return &g_sata_devices[device_id];
}

/**
 * Cleanup SATA subsystem
 */
void sata_cleanup(void) {
    // Reset all devices
    memset(g_sata_devices, 0, sizeof(g_sata_devices));
    g_device_count = 0;
    g_initialized = 0;
}

/**
 * Get device capacity in bytes
 */
uint64_t sata_get_capacity(uint32_t device_id) {
    if (device_id >= g_device_count) {
        return 0;
    }
    
    sata_device_t *device = &g_sata_devices[device_id];
    return device->sectors * device->sector_size;
}

/**
 * Check if device supports 48-bit LBA
 */
int sata_supports_lba48(uint32_t device_id) {
    if (device_id >= g_device_count) {
        return 0;
    }
    
    sata_device_t *device = &g_sata_devices[device_id];
    return device->sectors > 0x0FFFFFFFULL;
}

/**
 * Get device model string
 */
const char *sata_get_model(uint32_t device_id) {
    if (device_id >= g_device_count) {
        return NULL;
    }
    
    return g_sata_devices[device_id].model;
}

/**
 * Get device serial number
 */
const char *sata_get_serial(uint32_t device_id) {
    if (device_id >= g_device_count) {
        return NULL;
    }
    
    return g_sata_devices[device_id].serial;
}
