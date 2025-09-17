
// ===================================================================
// DESC: Generic block device driver interface for BDI Kernel
//       Provides common interface for all block storage devices
// ===================================================================

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// Block device constants
#define BDI_BLOCK_MAX_DEVICES       64
#define BDI_BLOCK_DEFAULT_SIZE      512
#define BDI_BLOCK_MAX_SECTORS       0xFFFFFFFF
#define BDI_BLOCK_NAME_MAX          32

// Block device types
#define BDI_BLOCK_TYPE_UNKNOWN      0
#define BDI_BLOCK_TYPE_HDD          1
#define BDI_BLOCK_TYPE_SSD          2
#define BDI_BLOCK_TYPE_NVME         3
#define BDI_BLOCK_TYPE_RAMDISK      4
#define BDI_BLOCK_TYPE_CDROM        5
#define BDI_BLOCK_TYPE_FLOPPY       6

// Block device flags
#define BDI_BLOCK_FLAG_READONLY     0x01
#define BDI_BLOCK_FLAG_REMOVABLE    0x02
#define BDI_BLOCK_FLAG_HOTPLUG      0x04
#define BDI_BLOCK_FLAG_ROTATIONAL   0x08

// Block device operations structure
typedef struct bdi_block_ops {
    int (*read)(void *device, uint64_t sector, uint32_t count, void *buffer);
    int (*write)(void *device, uint64_t sector, uint32_t count, const void *buffer);
    int (*flush)(void *device);
    int (*trim)(void *device, uint64_t sector, uint32_t count);
    int (*get_info)(void *device, void *info);
    int (*ioctl)(void *device, uint32_t cmd, void *arg);
} bdi_block_ops_t;

// Block device structure
typedef struct bdi_block_device {
    uint32_t device_id;                     // Unique device ID
    char name[BDI_BLOCK_NAME_MAX];          // Device name
    uint32_t type;                          // Device type
    uint32_t flags;                         // Device flags
    uint64_t total_sectors;                 // Total number of sectors
    uint32_t sector_size;                   // Sector size in bytes
    uint32_t max_sectors_per_request;       // Maximum sectors per I/O request
    void *private_data;                     // Driver private data
    bdi_block_ops_t *ops;                   // Device operations
    uint32_t ref_count;                     // Reference count
    uint8_t active;                         // Device active flag
} bdi_block_device_t;

// Block I/O request structure
typedef struct bdi_block_request {
    uint32_t device_id;                     // Target device ID
    uint64_t sector;                        // Starting sector
    uint32_t count;                         // Number of sectors
    void *buffer;                           // Data buffer
    uint32_t flags;                         // Request flags
    int (*callback)(struct bdi_block_request *req, int status); // Completion callback
    void *private_data;                     // Request private data
} bdi_block_request_t;

// Global block device registry
static bdi_block_device_t g_block_devices[BDI_BLOCK_MAX_DEVICES];
static uint32_t g_device_count = 0;
static uint32_t g_next_device_id = 1;
static uint8_t g_block_initialized = 0;

// Function prototypes
int bdi_block_init(void);
int bdi_block_register_device(bdi_block_device_t *device);
int bdi_block_unregister_device(uint32_t device_id);
bdi_block_device_t *bdi_block_get_device(uint32_t device_id);
bdi_block_device_t *bdi_block_find_device(const char *name);
int bdi_block_read(uint32_t device_id, uint64_t sector, uint32_t count, void *buffer);
int bdi_block_write(uint32_t device_id, uint64_t sector, uint32_t count, const void *buffer);
int bdi_block_flush(uint32_t device_id);
int bdi_block_trim(uint32_t device_id, uint64_t sector, uint32_t count);
int bdi_block_get_info(uint32_t device_id, void *info);
int bdi_block_ioctl(uint32_t device_id, uint32_t cmd, void *arg);
uint32_t bdi_block_get_device_count(void);
int bdi_block_list_devices(uint32_t *device_ids, uint32_t max_count);
void bdi_block_cleanup(void);

/**
 * Initialize block device subsystem
 */
int bdi_block_init(void) {
    if (g_block_initialized) {
        return 0;
    }
    
    // Clear device registry
    memset(g_block_devices, 0, sizeof(g_block_devices));
    g_device_count = 0;
    g_next_device_id = 1;
    g_block_initialized = 1;
    
    return 0;
}

/**
 * Register a block device
 */
int bdi_block_register_device(bdi_block_device_t *device) {
    if (!g_block_initialized || !device || !device->ops) {
        return -1;
    }
    
    if (g_device_count >= BDI_BLOCK_MAX_DEVICES) {
        return -1; // No free slots
    }
    
    // Find free slot
    bdi_block_device_t *slot = NULL;
    for (uint32_t i = 0; i < BDI_BLOCK_MAX_DEVICES; i++) {
        if (!g_block_devices[i].active) {
            slot = &g_block_devices[i];
            break;
        }
    }
    
    if (!slot) {
        return -1; // No free slots
    }
    
    // Copy device structure
    memcpy(slot, device, sizeof(bdi_block_device_t));
    
    // Assign device ID if not set
    if (slot->device_id == 0) {
        slot->device_id = g_next_device_id++;
    }
    
    // Set default values
    if (slot->sector_size == 0) {
        slot->sector_size = BDI_BLOCK_DEFAULT_SIZE;
    }
    
    if (slot->max_sectors_per_request == 0) {
        slot->max_sectors_per_request = 256; // Default 128KB
    }
    
    slot->ref_count = 0;
    slot->active = 1;
    
    g_device_count++;
    return slot->device_id;
}

/**
 * Unregister a block device
 */
int bdi_block_unregister_device(uint32_t device_id) {
    if (!g_block_initialized) {
        return -1;
    }
    
    bdi_block_device_t *device = bdi_block_get_device(device_id);
    if (!device) {
        return -1; // Device not found
    }
    
    if (device->ref_count > 0) {
        return -1; // Device still in use
    }
    
    // Clear device structure
    memset(device, 0, sizeof(bdi_block_device_t));
    g_device_count--;
    
    return 0;
}

/**
 * Get block device by ID
 */
bdi_block_device_t *bdi_block_get_device(uint32_t device_id) {
    if (!g_block_initialized) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < BDI_BLOCK_MAX_DEVICES; i++) {
        if (g_block_devices[i].active && g_block_devices[i].device_id == device_id) {
            return &g_block_devices[i];
        }
    }
    
    return NULL;
}

/**
 * Find block device by name
 */
bdi_block_device_t *bdi_block_find_device(const char *name) {
    if (!g_block_initialized || !name) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < BDI_BLOCK_MAX_DEVICES; i++) {
        if (g_block_devices[i].active && 
            strcmp(g_block_devices[i].name, name) == 0) {
            return &g_block_devices[i];
        }
    }
    
    return NULL;
}

/**
 * Read sectors from block device
 */
int bdi_block_read(uint32_t device_id, uint64_t sector, uint32_t count, void *buffer) {
    if (!buffer || count == 0) {
        return -1;
    }
    
    bdi_block_device_t *device = bdi_block_get_device(device_id);
    if (!device || !device->ops || !device->ops->read) {
        return -1;
    }
    
    // Check bounds
    if (sector + count > device->total_sectors) {
        return -1; // Out of bounds
    }
    
    // Check request size limit
    if (count > device->max_sectors_per_request) {
        return -1; // Request too large
    }
    
    // Check read-only flag for consistency
    // (reads are always allowed)
    
    return device->ops->read(device->private_data, sector, count, buffer);
}

/**
 * Write sectors to block device
 */
int bdi_block_write(uint32_t device_id, uint64_t sector, uint32_t count, const void *buffer) {
    if (!buffer || count == 0) {
        return -1;
    }
    
    bdi_block_device_t *device = bdi_block_get_device(device_id);
    if (!device || !device->ops || !device->ops->write) {
        return -1;
    }
    
    // Check read-only flag
    if (device->flags & BDI_BLOCK_FLAG_READONLY) {
        return -1; // Read-only device
    }
    
    // Check bounds
    if (sector + count > device->total_sectors) {
        return -1; // Out of bounds
    }
    
    // Check request size limit
    if (count > device->max_sectors_per_request) {
        return -1; // Request too large
    }
    
    return device->ops->write(device->private_data, sector, count, buffer);
}

/**
 * Flush block device cache
 */
int bdi_block_flush(uint32_t device_id) {
    bdi_block_device_t *device = bdi_block_get_device(device_id);
    if (!device || !device->ops) {
        return -1;
    }
    
    if (device->ops->flush) {
        return device->ops->flush(device->private_data);
    }
    
    return 0; // No flush operation, assume success
}

/**
 * Trim/discard sectors on block device
 */
int bdi_block_trim(uint32_t device_id, uint64_t sector, uint32_t count) {
    if (count == 0) {
        return -1;
    }
    
    bdi_block_device_t *device = bdi_block_get_device(device_id);
    if (!device || !device->ops) {
        return -1;
    }
    
    // Check read-only flag
    if (device->flags & BDI_BLOCK_FLAG_READONLY) {
        return -1; // Read-only device
    }
    
    // Check bounds
    if (sector + count > device->total_sectors) {
        return -1; // Out of bounds
    }
    
    if (device->ops->trim) {
        return device->ops->trim(device->private_data, sector, count);
    }
    
    return 0; // No trim operation, assume success
}

/**
 * Get block device information
 */
int bdi_block_get_info(uint32_t device_id, void *info) {
    if (!info) {
        return -1;
    }
    
    bdi_block_device_t *device = bdi_block_get_device(device_id);
    if (!device) {
        return -1;
    }
    
    if (device->ops && device->ops->get_info) {
        return device->ops->get_info(device->private_data, info);
    }
    
    // Return basic device info
    memcpy(info, device, sizeof(bdi_block_device_t));
    return 0;
}

/**
 * Block device I/O control
 */
int bdi_block_ioctl(uint32_t device_id, uint32_t cmd, void *arg) {
    bdi_block_device_t *device = bdi_block_get_device(device_id);
    if (!device || !device->ops) {
        return -1;
    }
    
    if (device->ops->ioctl) {
        return device->ops->ioctl(device->private_data, cmd, arg);
    }
    
    return -1; // Not supported
}

/**
 * Get number of registered block devices
 */
uint32_t bdi_block_get_device_count(void) {
    return g_device_count;
}

/**
 * List all registered block devices
 */
int bdi_block_list_devices(uint32_t *device_ids, uint32_t max_count) {
    if (!device_ids || max_count == 0) {
        return -1;
    }
    
    uint32_t count = 0;
    for (uint32_t i = 0; i < BDI_BLOCK_MAX_DEVICES && count < max_count; i++) {
        if (g_block_devices[i].active) {
            device_ids[count] = g_block_devices[i].device_id;
            count++;
        }
    }
    
    return count;
}

/**
 * Get device capacity in bytes
 */
uint64_t bdi_block_get_capacity(uint32_t device_id) {
    bdi_block_device_t *device = bdi_block_get_device(device_id);
    if (!device) {
        return 0;
    }
    
    return device->total_sectors * device->sector_size;
}

/**
 * Check if device is read-only
 */
int bdi_block_is_readonly(uint32_t device_id) {
    bdi_block_device_t *device = bdi_block_get_device(device_id);
    if (!device) {
        return 1; // Assume read-only if device not found
    }
    
    return (device->flags & BDI_BLOCK_FLAG_READONLY) != 0;
}

/**
 * Check if device is removable
 */
int bdi_block_is_removable(uint32_t device_id) {
    bdi_block_device_t *device = bdi_block_get_device(device_id);
    if (!device) {
        return 0;
    }
    
    return (device->flags & BDI_BLOCK_FLAG_REMOVABLE) != 0;
}

/**
 * Get device type string
 */
const char *bdi_block_get_type_string(uint32_t device_id) {
    bdi_block_device_t *device = bdi_block_get_device(device_id);
    if (!device) {
        return "unknown";
    }
    
    switch (device->type) {
        case BDI_BLOCK_TYPE_HDD:
            return "hdd";
        case BDI_BLOCK_TYPE_SSD:
            return "ssd";
        case BDI_BLOCK_TYPE_NVME:
            return "nvme";
        case BDI_BLOCK_TYPE_RAMDISK:
            return "ramdisk";
        case BDI_BLOCK_TYPE_CDROM:
            return "cdrom";
        case BDI_BLOCK_TYPE_FLOPPY:
            return "floppy";
        default:
            return "unknown";
    }
}

/**
 * Cleanup block device subsystem
 */
void bdi_block_cleanup(void) {
    // Clear all devices
    memset(g_block_devices, 0, sizeof(g_block_devices));
    g_device_count = 0;
    g_next_device_id = 1;
    g_block_initialized = 0;
}
