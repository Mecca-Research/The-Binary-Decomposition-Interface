
/**
 * @file fat32.h
 * @brief FAT32 filesystem with zero-copy I/O
 * 
 * Phase 5: Storage I/O Fast Paths
 */

#ifndef BDI_FAT32_H
#define BDI_FAT32_H

#include <stdint.h>
#include <stddef.h>

/* FAT32 boot sector */
struct fat32_boot_sector {
    uint8_t jmp[3];
    char oem[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t num_fats;
    uint16_t root_entries;
    uint16_t total_sectors_16;
    uint8_t media_type;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t fat_size_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t backup_boot_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    char volume_label[11];
    char fs_type[8];
} __attribute__((packed));

/* FAT32 directory entry */
struct fat32_dirent {
    char name[11];
    uint8_t attr;
    uint8_t reserved;
    uint8_t ctime_tenth;
    uint16_t ctime;
    uint16_t cdate;
    uint16_t adate;
    uint16_t cluster_hi;
    uint16_t mtime;
    uint16_t mdate;
    uint16_t cluster_lo;
    uint32_t size;
} __attribute__((packed));

/* FAT32 filesystem */
struct fat32_fs {
    struct fat32_boot_sector boot;
    uint32_t *fat;
    uint32_t fat_size;
    uint32_t root_cluster;
    uint32_t data_start;
    uint32_t cluster_size;
    void *device;
    int (*read_sectors)(void *dev, uint64_t lba, void *buf, size_t count);
    int (*write_sectors)(void *dev, uint64_t lba, const void *buf, size_t count);
};

/* FAT32 file */
struct fat32_file {
    struct fat32_fs *fs;
    uint32_t first_cluster;
    uint32_t current_cluster;
    uint32_t size;
    uint32_t position;
};

/* Function prototypes */
int fat32_mount(struct fat32_fs *fs, void *device,
                int (*read_fn)(void *, uint64_t, void *, size_t),
                int (*write_fn)(void *, uint64_t, const void *, size_t));
int fat32_open(struct fat32_fs *fs, const char *path, struct fat32_file *file);
int fat32_read(struct fat32_file *file, void *buf, size_t count);
int fat32_write(struct fat32_file *file, const void *buf, size_t count);
int fat32_close(struct fat32_file *file);
int fat32_readdir(struct fat32_fs *fs, uint32_t cluster, struct fat32_dirent *entries, size_t max_entries);
void fat32_unmount(struct fat32_fs *fs);

#endif /* BDI_FAT32_H */
