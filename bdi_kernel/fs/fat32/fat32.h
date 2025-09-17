
// ===================================================================
// DESC: FAT32 filesystem implementation header
// ===================================================================
#ifndef AEON_FAT32_H
#define AEON_FAT32_H

#include "../vfs/vfs.h"
#include <stdint.h>
#include <stdbool.h>

// --- FAT32 Constants ---
#define FAT32_SECTOR_SIZE       512
#define FAT32_MAX_FILENAME      255
#define FAT32_DIR_ENTRY_SIZE    32
#define FAT32_LFN_ENTRY_SIZE    32

// --- FAT32 Boot Sector ---
typedef struct {
    uint8_t  jmp_boot[3];           // Jump instruction
    uint8_t  oem_name[8];           // OEM name
    uint16_t bytes_per_sector;      // Bytes per sector (usually 512)
    uint8_t  sectors_per_cluster;   // Sectors per cluster
    uint16_t reserved_sectors;      // Reserved sectors (usually 32)
    uint8_t  num_fats;              // Number of FATs (usually 2)
    uint16_t root_entries;          // Root directory entries (0 for FAT32)
    uint16_t total_sectors_16;      // Total sectors (0 for FAT32)
    uint8_t  media_type;            // Media type (0xF8 for hard disk)
    uint16_t fat_size_16;           // Sectors per FAT (0 for FAT32)
    uint16_t sectors_per_track;     // Sectors per track
    uint16_t num_heads;             // Number of heads
    uint32_t hidden_sectors;        // Hidden sectors
    uint32_t total_sectors_32;      // Total sectors
    uint32_t fat_size_32;           // Sectors per FAT
    uint16_t ext_flags;             // Extended flags
    uint16_t fs_version;            // Filesystem version
    uint32_t root_cluster;          // Root directory cluster (usually 2)
    uint16_t fs_info;               // FSInfo sector
    uint16_t backup_boot_sector;    // Backup boot sector
    uint8_t  reserved[12];          // Reserved
    uint8_t  drive_number;          // Drive number
    uint8_t  reserved1;             // Reserved
    uint8_t  boot_signature;        // Boot signature (0x29)
    uint32_t volume_id;             // Volume ID
    uint8_t  volume_label[11];      // Volume label
    uint8_t  fs_type[8];            // Filesystem type ("FAT32   ")
    uint8_t  boot_code[420];        // Boot code
    uint16_t boot_sector_signature; // Boot sector signature (0xAA55)
} __attribute__((packed)) fat32_boot_sector_t;

// --- FAT32 Directory Entry ---
typedef struct {
    uint8_t  name[11];              // 8.3 filename
    uint8_t  attr;                  // File attributes
    uint8_t  nt_reserved;           // NT reserved
    uint8_t  create_time_tenth;     // Creation time (tenths of second)
    uint16_t create_time;           // Creation time
    uint16_t create_date;           // Creation date
    uint16_t last_access_date;      // Last access date
    uint16_t first_cluster_hi;      // High 16 bits of first cluster
    uint16_t write_time;            // Write time
    uint16_t write_date;            // Write date
    uint16_t first_cluster_lo;      // Low 16 bits of first cluster
    uint32_t file_size;             // File size
} __attribute__((packed)) fat32_dir_entry_t;

// --- FAT32 Long Filename Entry ---
typedef struct {
    uint8_t  order;                 // Order of this entry
    uint16_t name1[5];              // First 5 characters
    uint8_t  attr;                  // Attributes (always 0x0F)
    uint8_t  type;                  // Type (always 0)
    uint8_t  checksum;              // Checksum of short name
    uint16_t name2[6];              // Next 6 characters
    uint16_t first_cluster_lo;      // Always 0
    uint16_t name3[2];              // Last 2 characters
} __attribute__((packed)) fat32_lfn_entry_t;

// --- File Attributes ---
#define FAT32_ATTR_READ_ONLY    0x01
#define FAT32_ATTR_HIDDEN       0x02
#define FAT32_ATTR_SYSTEM       0x04
#define FAT32_ATTR_VOLUME_ID    0x08
#define FAT32_ATTR_DIRECTORY    0x10
#define FAT32_ATTR_ARCHIVE      0x20
#define FAT32_ATTR_LONG_NAME    0x0F

// --- Special Cluster Values ---
#define FAT32_CLUSTER_FREE      0x00000000
#define FAT32_CLUSTER_RESERVED  0x0FFFFFF0
#define FAT32_CLUSTER_BAD       0x0FFFFFF7
#define FAT32_CLUSTER_EOC       0x0FFFFFF8  // End of cluster chain

// --- FAT32 Filesystem Data ---
typedef struct {
    vfs_device_t* device;
    fat32_boot_sector_t boot_sector;
    
    // Calculated values
    uint32_t fat_start_sector;      // First FAT sector
    uint32_t data_start_sector;     // First data sector
    uint32_t cluster_size;          // Cluster size in bytes
    uint32_t total_clusters;        // Total number of clusters
    
    // FAT cache (simple single-sector cache)
    uint32_t cached_fat_sector;
    uint8_t  fat_cache[FAT32_SECTOR_SIZE];
    bool     fat_cache_dirty;
} fat32_fs_t;

// --- FAT32 Node Data ---
typedef struct {
    uint32_t first_cluster;         // First cluster of file/directory
    uint32_t current_cluster;       // Current cluster for sequential access
    uint32_t cluster_offset;        // Offset within current cluster
    bool     is_directory;          // True if this is a directory
} fat32_node_data_t;

// --- Function Declarations ---

// Filesystem operations
int fat32_mount(vfs_mount_t* mount, void* device);
int fat32_unmount(vfs_mount_t* mount);
vfs_node_t* fat32_get_root(vfs_mount_t* mount);
int fat32_sync(vfs_mount_t* mount);

// Node operations
int fat32_read(vfs_node_t* node, uint64_t offset, uint64_t size, void* buffer);
int fat32_write(vfs_node_t* node, uint64_t offset, uint64_t size, const void* buffer);
vfs_node_t* fat32_finddir(vfs_node_t* node, const char* name);
vfs_node_t** fat32_readdir(vfs_node_t* node, uint32_t* count);
int fat32_create(vfs_node_t* parent, const char* name, uint32_t type);
int fat32_unlink(vfs_node_t* parent, const char* name);
int fat32_mkdir(vfs_node_t* parent, const char* name);
int fat32_rmdir(vfs_node_t* parent, const char* name);

// Cluster operations
uint32_t fat32_get_next_cluster(fat32_fs_t* fs, uint32_t cluster);
int fat32_set_cluster(fat32_fs_t* fs, uint32_t cluster, uint32_t value);
uint32_t fat32_allocate_cluster(fat32_fs_t* fs);
int fat32_free_cluster_chain(fat32_fs_t* fs, uint32_t first_cluster);

// Utility functions
uint32_t fat32_cluster_to_sector(fat32_fs_t* fs, uint32_t cluster);
int fat32_read_cluster(fat32_fs_t* fs, uint32_t cluster, void* buffer);
int fat32_write_cluster(fat32_fs_t* fs, uint32_t cluster, const void* buffer);
int fat32_flush_fat_cache(fat32_fs_t* fs);

// Directory operations
int fat32_parse_directory(fat32_fs_t* fs, uint32_t cluster, vfs_node_t*** nodes, uint32_t* count);
int fat32_find_directory_entry(fat32_fs_t* fs, uint32_t cluster, const char* name, fat32_dir_entry_t* entry);
int fat32_create_directory_entry(fat32_fs_t* fs, uint32_t cluster, const char* name, uint32_t first_cluster, uint32_t size, uint8_t attr);

// Filename conversion
void fat32_name_to_83(const char* name, char* name83);
void fat32_name_from_83(const char* name83, char* name);
uint8_t fat32_lfn_checksum(const char* name83);

// Registration
int fat32_register(void);

#endif // AEON_FAT32_H
