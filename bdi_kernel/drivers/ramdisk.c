
// ===================================================================
// DESC: RAM Disk driver for BDI Kernel
//       Provides in-memory block storage for testing and temporary files
// ===================================================================

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// RAM disk constants
#define BDI_RAMDISK_MAX_DEVICES     8
#define BDI_RAMDISK_DEFAULT_SIZE    (16 * 1024 * 1024)  // 16MB
#define BDI_RAMDISK_SECTOR_SIZE     512
#define BDI_RAMDISK_MAX_SIZE        (1024 * 1024 * 1024) // 1GB

// RAM disk device structure
typedef struct {
    uint32_t device_id;         // Device ID
    char name[32];              // Device name
    uint8_t *memory;            // Memory buffer
    uint64_t size;              // Total size in bytes
    uint64_t sectors;           // Number of sectors
    uint32_t sector_size;       // Sector size
    uint8_t active;             // Device active flag
} bdi_ramdisk_t;

// Global RAM disk state
static bdi_ramdisk_t g_ramdisks[BDI_RAMDISK_MAX_DEVICES];
static uint32_t g_ramdisk_count = 0;
static uint8_t g_ramdisk_initialized = 0;

// Forward declarations for block device operations
int bdi_ramdisk_read(void *device, uint64_t sector, uint32_t count, void *buffer);
int bdi_ramdisk_write(void *device, uint64_t sector, uint32_t count, const void *buffer);
int bdi_ramdisk_flush(void *device);
int bdi_ramdisk_trim(void *device, uint64_t sector, uint32_t count);
int bdi_ramdisk_get_info(void *device, void *info);
int bdi_ramdisk_ioctl(void *device, uint32_t cmd, void *arg);

// Block device operations structure
static struct bdi_block_ops g_ramdisk_ops = {
    .read = bdi_ramdisk_read,
    .write = bdi_ramdisk_write,
    .flush = bdi_ramdisk_flush,
    .trim = bdi_ramdisk_trim,
    .get_info = bdi_ramdisk_get_info,
    .ioctl = bdi_ramdisk_ioctl
};

// Function prototypes
int bdi_ramdisk_init(void);
int bdi_ramdisk_create(const char *name, uint64_t size);
int bdi_ramdisk_destroy(uint32_t device_id);
bdi_ramdisk_t *bdi_ramdisk_get(uint32_t device_id);
uint32_t bdi_ramdisk_get_count(void);
void bdi_ramdisk_cleanup(void);

/**
 * Initialize RAM disk subsystem
 */
int bdi_ramdisk_init(void) {
    if (g_ramdisk_initialized) {
        return 0;
    }
    
    // Clear RAM disk array
    memset(g_ramdisks, 0, sizeof(g_ramdisks));
    g_ramdisk_count = 0;
    g_ramdisk_initialized = 1;
    
    return 0;
}

/**
 * Create a RAM disk
 */
int bdi_ramdisk_create(const char *name, uint64_t size) {
    if (!g_ramdisk_initialized || !name || size == 0 || size > BDI_RAMDISK_MAX_SIZE) {
        return -1;
    }
    
    if (g_ramdisk_count >= BDI_RAMDISK_MAX_DEVICES) {
        return -1; // No free slots
    }
    
    // Find free slot
    bdi_ramdisk_t *ramdisk = NULL;
    for (uint32_t i = 0; i < BDI_RAMDISK_MAX_DEVICES; i++) {
        if (!g_ramdisks[i].active) {
            ramdisk = &g_ramdisks[i];
            break;
        }
    }
    
    if (!ramdisk) {
        return -1; // No free slots
    }
    
    // Allocate memory for RAM disk
    uint8_t *memory = (uint8_t *)malloc(size);
    if (!memory) {
        return -1; // Memory allocation failed
    }
    
    // Initialize memory to zero
    memset(memory, 0, size);
    
    // Initialize RAM disk structure
    ramdisk->device_id = g_ramdisk_count + 1;
    strncpy(ramdisk->name, name, sizeof(ramdisk->name) - 1);
    ramdisk->name[sizeof(ramdisk->name) - 1] = '\0';
    ramdisk->memory = memory;
    ramdisk->size = size;
    ramdisk->sector_size = BDI_RAMDISK_SECTOR_SIZE;
    ramdisk->sectors = size / BDI_RAMDISK_SECTOR_SIZE;
    ramdisk->active = 1;
    
    g_ramdisk_count++;
    
    // Register with block device subsystem
    // (This would require including the block device header)
    /*
    bdi_block_device_t block_device;
    memset(&block_device, 0, sizeof(block_device));
    block_device.device_id = ramdisk->device_id;
    strncpy(block_device.name, name, sizeof(block_device.name) - 1);
    block_device.type = BDI_BLOCK_TYPE_RAMDISK;
    block_device.flags = 0; // Read-write
    block_device.total_sectors = ramdisk->sectors;
    block_device.sector_size = ramdisk->sector_size;
    block_device.max_sectors_per_request = ramdisk->sectors;
    block_device.private_data = ramdisk;
    block_device.ops = &g_ramdisk_ops;
    
    bdi_block_register_device(&block_device);
    */
    
    return ramdisk->device_id;
}

/**
 * Destroy a RAM disk
 */
int bdi_ramdisk_destroy(uint32_t device_id) {
    if (!g_ramdisk_initialized) {
        return -1;
    }
    
    bdi_ramdisk_t *ramdisk = bdi_ramdisk_get(device_id);
    if (!ramdisk) {
        return -1; // Device not found
    }
    
    // Unregister from block device subsystem
    // bdi_block_unregister_device(device_id);
    
    // Free memory
    if (ramdisk->memory) {
        free(ramdisk->memory);
    }
    
    // Clear structure
    memset(ramdisk, 0, sizeof(bdi_ramdisk_t));
    g_ramdisk_count--;
    
    return 0;
}

/**
 * Get RAM disk by device ID
 */
bdi_ramdisk_t *bdi_ramdisk_get(uint32_t device_id) {
    if (!g_ramdisk_initialized) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < BDI_RAMDISK_MAX_DEVICES; i++) {
        if (g_ramdisks[i].active && g_ramdisks[i].device_id == device_id) {
            return &g_ramdisks[i];
        }
    }
    
    return NULL;
}

/**
 * Read sectors from RAM disk
 */
int bdi_ramdisk_read(void *device, uint64_t sector, uint32_t count, void *buffer) {
    if (!device || !buffer || count == 0) {
        return -1;
    }
    
    bdi_ramdisk_t *ramdisk = (bdi_ramdisk_t *)device;
    
    // Check bounds
    if (sector + count > ramdisk->sectors) {
        return -1; // Out of bounds
    }
    
    // Calculate byte offset and size
    uint64_t offset = sector * ramdisk->sector_size;
    uint64_t size = count * ramdisk->sector_size;
    
    // Copy data from RAM disk to buffer
    memcpy(buffer, ramdisk->memory + offset, size);
    
    return 0;
}

/**
 * Write sectors to RAM disk
 */
int bdi_ramdisk_write(void *device, uint64_t sector, uint32_t count, const void *buffer) {
    if (!device || !buffer || count == 0) {
        return -1;
    }
    
    bdi_ramdisk_t *ramdisk = (bdi_ramdisk_t *)device;
    
    // Check bounds
    if (sector + count > ramdisk->sectors) {
        return -1; // Out of bounds
    }
    
    // Calculate byte offset and size
    uint64_t offset = sector * ramdisk->sector_size;
    uint64_t size = count * ramdisk->sector_size;
    
    // Copy data from buffer to RAM disk
    memcpy(ramdisk->memory + offset, buffer, size);
    
    return 0;
}

/**
 * Flush RAM disk (no-op for RAM disk)
 */
int bdi_ramdisk_flush(void *device) {
    (void)device; // Unused parameter
    
    // RAM disk doesn't need flushing
    return 0;
}

/**
 * Trim sectors on RAM disk
 */
int bdi_ramdisk_trim(void *device, uint64_t sector, uint32_t count) {
    if (!device || count == 0) {
        return -1;
    }
    
    bdi_ramdisk_t *ramdisk = (bdi_ramdisk_t *)device;
    
    // Check bounds
    if (sector + count > ramdisk->sectors) {
        return -1; // Out of bounds
    }
    
    // Calculate byte offset and size
    uint64_t offset = sector * ramdisk->sector_size;
    uint64_t size = count * ramdisk->sector_size;
    
    // Zero out the trimmed sectors
    memset(ramdisk->memory + offset, 0, size);
    
    return 0;
}

/**
 * Get RAM disk information
 */
int bdi_ramdisk_get_info(void *device, void *info) {
    if (!device || !info) {
        return -1;
    }
    
    bdi_ramdisk_t *ramdisk = (bdi_ramdisk_t *)device;
    
    // Copy RAM disk structure to info buffer
    memcpy(info, ramdisk, sizeof(bdi_ramdisk_t));
    
    return 0;
}

/**
 * RAM disk I/O control
 */
int bdi_ramdisk_ioctl(void *device, uint32_t cmd, void *arg) {
    if (!device) {
        return -1;
    }
    
    bdi_ramdisk_t *ramdisk = (bdi_ramdisk_t *)device;
    
    switch (cmd) {
        case 0x1000: // Get size
            if (arg) {
                *((uint64_t *)arg) = ramdisk->size;
                return 0;
            }
            break;
            
        case 0x1001: // Get sector count
            if (arg) {
                *((uint64_t *)arg) = ramdisk->sectors;
                return 0;
            }
            break;
            
        case 0x1002: // Get sector size
            if (arg) {
                *((uint32_t *)arg) = ramdisk->sector_size;
                return 0;
            }
            break;
            
        case 0x1003: // Clear all data
            memset(ramdisk->memory, 0, ramdisk->size);
            return 0;
            
        default:
            break;
    }
    
    return -1; // Unsupported command
}

/**
 * Get number of active RAM disks
 */
uint32_t bdi_ramdisk_get_count(void) {
    return g_ramdisk_count;
}

/**
 * Get RAM disk usage statistics
 */
int bdi_ramdisk_get_stats(uint32_t device_id, uint64_t *total_size, uint64_t *used_size) {
    bdi_ramdisk_t *ramdisk = bdi_ramdisk_get(device_id);
    if (!ramdisk) {
        return -1;
    }
    
    if (total_size) {
        *total_size = ramdisk->size;
    }
    
    if (used_size) {
        // For RAM disk, we can't easily determine used size
        // without scanning the entire memory, so return total size
        *used_size = ramdisk->size;
    }
    
    return 0;
}

/**
 * Create default RAM disk for testing
 */
int bdi_ramdisk_create_default(void) {
    return bdi_ramdisk_create("ramdisk0", BDI_RAMDISK_DEFAULT_SIZE);
}

/**
 * List all RAM disks
 */
int bdi_ramdisk_list(uint32_t *device_ids, char names[][32], uint32_t max_count) {
    if (!device_ids || max_count == 0) {
        return -1;
    }
    
    uint32_t count = 0;
    for (uint32_t i = 0; i < BDI_RAMDISK_MAX_DEVICES && count < max_count; i++) {
        if (g_ramdisks[i].active) {
            device_ids[count] = g_ramdisks[i].device_id;
            if (names) {
                strncpy(names[count], g_ramdisks[i].name, 31);
                names[count][31] = '\0';
            }
            count++;
        }
    }
    
    return count;
}

/**
 * Cleanup RAM disk subsystem
 */
void bdi_ramdisk_cleanup(void) {
    if (!g_ramdisk_initialized) {
        return;
    }
    
    // Destroy all RAM disks
    for (uint32_t i = 0; i < BDI_RAMDISK_MAX_DEVICES; i++) {
        if (g_ramdisks[i].active) {
            bdi_ramdisk_destroy(g_ramdisks[i].device_id);
        }
    }
    
    g_ramdisk_count = 0;
    g_ramdisk_initialized = 0;
}
