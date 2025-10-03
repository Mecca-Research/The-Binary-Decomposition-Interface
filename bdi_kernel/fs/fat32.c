
/**
 * @file fat32.c
 * @brief FAT32 filesystem implementation
 * 
 * Phase 5: Storage I/O Fast Paths
 */

#include "fat32.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

[[maybe_unused]] static int fat32_debug = 0;

#define FAT32_EOC 0x0FFFFFF8

/**
 * @brief Get next cluster in chain
 */
static uint32_t fat32_next_cluster(struct fat32_fs *fs, uint32_t cluster) {
    if (cluster >= 0x0FFFFFF8) {
        return 0;
    }
    return fs->fat[cluster] & 0x0FFFFFFF;
}

/**
 * @brief Mount FAT32 filesystem
 */
int fat32_mount(struct fat32_fs *fs, void *device,
                int (*read_fn)(void *, uint64_t, void *, size_t),
                int (*write_fn)(void *, uint64_t, const void *, size_t)) {
    int ret;
    
    if (!fs || !device || !read_fn) {
        return -EINVAL;
    }
    
    memset(fs, 0, sizeof(*fs));
    fs->device = device;
    fs->read_sectors = read_fn;
    fs->write_sectors = write_fn;
    
    /* Read boot sector */
    ret = read_fn(device, 0, &fs->boot, 1);
    if (ret < 0) {
        return ret;
    }
    
    /* Validate FAT32 */
    if (memcmp(fs->boot.fs_type, "FAT32   ", 8) != 0) {
        return -EINVAL;
    }
    
    /* Calculate filesystem parameters */
    fs->fat_size = fs->boot.fat_size_32;
    fs->root_cluster = fs->boot.root_cluster;
    fs->data_start = fs->boot.reserved_sectors + (fs->boot.num_fats * fs->fat_size);
    fs->cluster_size = fs->boot.sectors_per_cluster * fs->boot.bytes_per_sector;
    
    /* Allocate and read FAT */
    fs->fat = malloc(fs->fat_size * fs->boot.bytes_per_sector);
    if (!fs->fat) {
        return -ENOMEM;
    }
    
    ret = read_fn(device, fs->boot.reserved_sectors, fs->fat, fs->fat_size);
    if (ret < 0) {
        free(fs->fat);
        return ret;
    }
    
    return 0;
}

/**
 * @brief Open file
 */
int fat32_open(struct fat32_fs *fs, const char *path, struct fat32_file *file) {
    /* TODO: Parse path and find file */
    /* TODO: Read directory entries */
    /* TODO: Find matching entry */
    
    if (!fs || !path || !file) {
        return -EINVAL;
    }
    
    memset(file, 0, sizeof(*file));
    file->fs = fs;
    file->first_cluster = fs->root_cluster;
    file->current_cluster = fs->root_cluster;
    
    return -ENOSYS;
}

/**
 * @brief Read from file
 */
int fat32_read(struct fat32_file *file, void *buf, size_t count) {
    uint32_t cluster_offset, bytes_read = 0;
    uint8_t *buffer = buf;
    
    if (!file || !buf) {
        return -EINVAL;
    }
    
    while (count > 0 && file->current_cluster < FAT32_EOC) {
        cluster_offset = file->position % file->fs->cluster_size;
        uint32_t bytes_in_cluster = file->fs->cluster_size - cluster_offset;
        uint32_t to_read = (count < bytes_in_cluster) ? count : bytes_in_cluster;
        
        /* Calculate sector and intra-sector offset */
        uint64_t sector = file->fs->data_start + 
                         ((file->current_cluster - 2) * file->fs->boot.sectors_per_cluster);
        sector += cluster_offset / file->fs->boot.bytes_per_sector;
        uint32_t sector_offset = cluster_offset % file->fs->boot.bytes_per_sector;
        
        /*
         * BUG FIX: Honor intra-sector offsets when reading FAT32 clusters.
         * When a read begins in the middle of a sector, we must:
         * 1. Read the sector(s) into a temporary buffer
         * 2. Skip the unwanted leading bytes (sector_offset)
         * 3. Copy only the desired data to the output buffer
         */
        if (sector_offset != 0 || to_read < file->fs->boot.bytes_per_sector) {
            /* Read needs to handle partial sector */
            uint32_t sectors_needed = (sector_offset + to_read + file->fs->boot.bytes_per_sector - 1) / 
                                     file->fs->boot.bytes_per_sector;
            uint8_t temp_buf[4096];  /* Temporary buffer for staging sectors */
            
            if (sectors_needed * file->fs->boot.bytes_per_sector > sizeof(temp_buf)) {
                return -EINVAL;  /* Sanity check */
            }
            
            int ret = file->fs->read_sectors(file->fs->device, sector, temp_buf, sectors_needed);
            if (ret < 0) {
                return ret;
            }
            
            /* Copy only the desired portion, skipping sector_offset bytes */
            memcpy(buffer + bytes_read, temp_buf + sector_offset, to_read);
        } else {
            /* Aligned read - can read directly into output buffer */
            int ret = file->fs->read_sectors(file->fs->device, sector, 
                                            buffer + bytes_read, 
                                            (to_read + file->fs->boot.bytes_per_sector - 1) / 
                                            file->fs->boot.bytes_per_sector);
            if (ret < 0) {
                return ret;
            }
        }
        
        bytes_read += to_read;
        count -= to_read;
        file->position += to_read;
        
        /* Move to next cluster if needed */
        if (file->position % file->fs->cluster_size == 0) {
            file->current_cluster = fat32_next_cluster(file->fs, file->current_cluster);
        }
    }
    
    return bytes_read;
}

/**
 * @brief Write to file
 */
int fat32_write(struct fat32_file *file, const void *buf, size_t count) {
    /* TODO: Implement write */
    return -ENOSYS;
}

/**
 * @brief Close file
 */
int fat32_close(struct fat32_file *file) {
    if (!file) {
        return -EINVAL;
    }
    
    memset(file, 0, sizeof(*file));
    return 0;
}

/**
 * @brief Read directory
 */
int fat32_readdir(struct fat32_fs *fs, uint32_t cluster, struct fat32_dirent *entries, size_t max_entries) {
    /* TODO: Implement readdir */
    return -ENOSYS;
}

/**
 * @brief Unmount filesystem
 */
void fat32_unmount(struct fat32_fs *fs) {
    if (!fs) {
        return;
    }
    
    free(fs->fat);
    memset(fs, 0, sizeof(*fs));
}
