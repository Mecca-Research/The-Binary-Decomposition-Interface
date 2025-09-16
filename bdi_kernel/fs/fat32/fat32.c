
// ===================================================================
// DESC: FAT32 filesystem implementation
// ===================================================================

#include "fat32.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// --- Global FAT32 Filesystem Structure ---
static vfs_filesystem_t fat32_filesystem = {
    .name = "fat32",
    .type = VFS_FS_FAT32,
    .mount = fat32_mount,
    .unmount = fat32_unmount,
    .get_root = fat32_get_root,
    .sync = fat32_sync
};

// --- Filesystem Operations ---

int fat32_mount(vfs_mount_t* mount, void* device) {
    vfs_device_t* dev = (vfs_device_t*)device;
    
    // Allocate filesystem data
    fat32_fs_t* fs = (fat32_fs_t*)malloc(sizeof(fat32_fs_t));
    if (!fs) {
        return VFS_ERROR_NO_MEMORY;
    }
    memset(fs, 0, sizeof(fat32_fs_t));
    fs->device = dev;
    fs->cached_fat_sector = 0xFFFFFFFF; // Invalid sector
    
    // Read boot sector
    if (dev->read_sectors(device, 0, 1, &fs->boot_sector) != 0) {
        free(fs);
        return VFS_ERROR_IO;
    }
    
    // Validate boot sector
    if (fs->boot_sector.boot_sector_signature != 0xAA55) {
        printf("FAT32: Invalid boot sector signature\n");
        free(fs);
        return VFS_ERROR_INVALID;
    }
    
    if (fs->boot_sector.bytes_per_sector != FAT32_SECTOR_SIZE) {
        printf("FAT32: Unsupported sector size: %u\n", fs->boot_sector.bytes_per_sector);
        free(fs);
        return VFS_ERROR_INVALID;
    }
    
    // Calculate filesystem parameters
    fs->fat_start_sector = fs->boot_sector.reserved_sectors;
    fs->data_start_sector = fs->fat_start_sector + (fs->boot_sector.num_fats * fs->boot_sector.fat_size_32);
    fs->cluster_size = fs->boot_sector.sectors_per_cluster * fs->boot_sector.bytes_per_sector;
    
    uint32_t data_sectors = fs->boot_sector.total_sectors_32 - fs->data_start_sector;
    fs->total_clusters = data_sectors / fs->boot_sector.sectors_per_cluster;
    
    printf("FAT32: Mounted filesystem\n");
    printf("  Sectors per cluster: %u\n", fs->boot_sector.sectors_per_cluster);
    printf("  Cluster size: %u bytes\n", fs->cluster_size);
    printf("  Total clusters: %u\n", fs->total_clusters);
    printf("  Root cluster: %u\n", fs->boot_sector.root_cluster);
    
    mount->fs = &fat32_filesystem;
    mount->mount_data = fs;
    
    return VFS_SUCCESS;
}

int fat32_unmount(vfs_mount_t* mount) {
    fat32_fs_t* fs = (fat32_fs_t*)mount->mount_data;
    if (fs) {
        fat32_flush_fat_cache(fs);
        free(fs);
    }
    return VFS_SUCCESS;
}

vfs_node_t* fat32_get_root(vfs_mount_t* mount) {
    fat32_fs_t* fs = (fat32_fs_t*)mount->mount_data;
    
    vfs_node_t* root = (vfs_node_t*)malloc(sizeof(vfs_node_t));
    if (!root) {
        return NULL;
    }
    memset(root, 0, sizeof(vfs_node_t));
    
    strcpy(root->name, "/");
    root->type = VFS_TYPE_DIR;
    root->permissions = VFS_PERM_READ | VFS_PERM_WRITE | VFS_PERM_EXEC;
    root->fs = &fat32_filesystem;
    root->mount = mount;
    
    // Allocate FAT32-specific data
    fat32_node_data_t* node_data = (fat32_node_data_t*)malloc(sizeof(fat32_node_data_t));
    if (!node_data) {
        free(root);
        return NULL;
    }
    node_data->first_cluster = fs->boot_sector.root_cluster;
    node_data->current_cluster = fs->boot_sector.root_cluster;
    node_data->cluster_offset = 0;
    node_data->is_directory = true;
    root->fs_data = node_data;
    
    // Set operations
    root->read = fat32_read;
    root->write = fat32_write;
    root->finddir = fat32_finddir;
    root->readdir = fat32_readdir;
    root->create = fat32_create;
    root->unlink = fat32_unlink;
    root->mkdir = fat32_mkdir;
    root->rmdir = fat32_rmdir;
    
    return root;
}

int fat32_sync(vfs_mount_t* mount) {
    fat32_fs_t* fs = (fat32_fs_t*)mount->mount_data;
    return fat32_flush_fat_cache(fs);
}

// --- Cluster Operations ---

uint32_t fat32_get_next_cluster(fat32_fs_t* fs, uint32_t cluster) {
    if (cluster < 2 || cluster >= fs->total_clusters + 2) {
        return FAT32_CLUSTER_EOC;
    }
    
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fs->fat_start_sector + (fat_offset / FAT32_SECTOR_SIZE);
    uint32_t entry_offset = fat_offset % FAT32_SECTOR_SIZE;
    
    // Check if we need to read a different FAT sector
    if (fs->cached_fat_sector != fat_sector) {
        // Flush current cache if dirty
        if (fat32_flush_fat_cache(fs) != VFS_SUCCESS) {
            return FAT32_CLUSTER_EOC;
        }
        
        // Read new sector
        if (fs->device->read_sectors(fs->device, fat_sector, 1, fs->fat_cache) != 0) {
            return FAT32_CLUSTER_EOC;
        }
        fs->cached_fat_sector = fat_sector;
        fs->fat_cache_dirty = false;
    }
    
    uint32_t next_cluster = *(uint32_t*)(fs->fat_cache + entry_offset) & 0x0FFFFFFF;
    return next_cluster;
}

int fat32_set_cluster(fat32_fs_t* fs, uint32_t cluster, uint32_t value) {
    if (cluster < 2 || cluster >= fs->total_clusters + 2) {
        return VFS_ERROR_INVALID;
    }
    
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fs->fat_start_sector + (fat_offset / FAT32_SECTOR_SIZE);
    uint32_t entry_offset = fat_offset % FAT32_SECTOR_SIZE;
    
    // Check if we need to read a different FAT sector
    if (fs->cached_fat_sector != fat_sector) {
        // Flush current cache if dirty
        if (fat32_flush_fat_cache(fs) != VFS_SUCCESS) {
            return VFS_ERROR_IO;
        }
        
        // Read new sector
        if (fs->device->read_sectors(fs->device, fat_sector, 1, fs->fat_cache) != 0) {
            return VFS_ERROR_IO;
        }
        fs->cached_fat_sector = fat_sector;
        fs->fat_cache_dirty = false;
    }
    
    // Update FAT entry (preserve upper 4 bits)
    uint32_t* entry = (uint32_t*)(fs->fat_cache + entry_offset);
    *entry = (*entry & 0xF0000000) | (value & 0x0FFFFFFF);
    fs->fat_cache_dirty = true;
    
    return VFS_SUCCESS;
}

uint32_t fat32_allocate_cluster(fat32_fs_t* fs) {
    // Simple allocation: scan from cluster 2
    for (uint32_t cluster = 2; cluster < fs->total_clusters + 2; cluster++) {
        uint32_t next = fat32_get_next_cluster(fs, cluster);
        if (next == FAT32_CLUSTER_FREE) {
            // Mark as end of chain
            if (fat32_set_cluster(fs, cluster, FAT32_CLUSTER_EOC) == VFS_SUCCESS) {
                return cluster;
            }
        }
    }
    return FAT32_CLUSTER_FREE; // No free clusters
}

int fat32_free_cluster_chain(fat32_fs_t* fs, uint32_t first_cluster) {
    uint32_t cluster = first_cluster;
    
    while (cluster >= 2 && cluster < FAT32_CLUSTER_RESERVED) {
        uint32_t next_cluster = fat32_get_next_cluster(fs, cluster);
        if (fat32_set_cluster(fs, cluster, FAT32_CLUSTER_FREE) != VFS_SUCCESS) {
            return VFS_ERROR_IO;
        }
        cluster = next_cluster;
    }
    
    return VFS_SUCCESS;
}

// --- Utility Functions ---

uint32_t fat32_cluster_to_sector(fat32_fs_t* fs, uint32_t cluster) {
    if (cluster < 2) {
        return 0;
    }
    return fs->data_start_sector + ((cluster - 2) * fs->boot_sector.sectors_per_cluster);
}

int fat32_read_cluster(fat32_fs_t* fs, uint32_t cluster, void* buffer) {
    uint32_t sector = fat32_cluster_to_sector(fs, cluster);
    return fs->device->read_sectors(fs->device, sector, fs->boot_sector.sectors_per_cluster, buffer);
}

int fat32_write_cluster(fat32_fs_t* fs, uint32_t cluster, const void* buffer) {
    uint32_t sector = fat32_cluster_to_sector(fs, cluster);
    return fs->device->write_sectors(fs->device, sector, fs->boot_sector.sectors_per_cluster, buffer);
}

int fat32_flush_fat_cache(fat32_fs_t* fs) {
    if (fs->fat_cache_dirty && fs->cached_fat_sector != 0xFFFFFFFF) {
        // Write to all FAT copies
        for (int i = 0; i < fs->boot_sector.num_fats; i++) {
            uint32_t fat_sector = fs->fat_start_sector + (i * fs->boot_sector.fat_size_32) + 
                                 (fs->cached_fat_sector - fs->fat_start_sector);
            if (fs->device->write_sectors(fs->device, fat_sector, 1, fs->fat_cache) != 0) {
                return VFS_ERROR_IO;
            }
        }
        fs->fat_cache_dirty = false;
    }
    return VFS_SUCCESS;
}

// --- Node Operations ---

int fat32_read(vfs_node_t* node, uint64_t offset, uint64_t size, void* buffer) {
    fat32_node_data_t* node_data = (fat32_node_data_t*)node->fs_data;
    fat32_fs_t* fs = (fat32_fs_t*)node->mount->mount_data;
    
    if (node_data->is_directory) {
        return VFS_ERROR_IS_DIR;
    }
    
    if (offset >= node->size) {
        return 0; // EOF
    }
    
    if (offset + size > node->size) {
        size = node->size - offset;
    }
    
    uint8_t* cluster_buffer = (uint8_t*)malloc(fs->cluster_size);
    if (!cluster_buffer) {
        return VFS_ERROR_NO_MEMORY;
    }
    
    uint64_t bytes_read = 0;
    uint32_t cluster = node_data->first_cluster;
    uint64_t cluster_offset = offset;
    
    // Skip to the correct cluster
    while (cluster_offset >= fs->cluster_size && cluster >= 2 && cluster < FAT32_CLUSTER_RESERVED) {
        cluster = fat32_get_next_cluster(fs, cluster);
        cluster_offset -= fs->cluster_size;
    }
    
    // Read data
    while (size > 0 && cluster >= 2 && cluster < FAT32_CLUSTER_RESERVED) {
        if (fat32_read_cluster(fs, cluster, cluster_buffer) != 0) {
            free(cluster_buffer);
            return VFS_ERROR_IO;
        }
        
        uint32_t bytes_to_copy = (size < fs->cluster_size - cluster_offset) ? 
                                size : (fs->cluster_size - cluster_offset);
        
        memcpy((uint8_t*)buffer + bytes_read, cluster_buffer + cluster_offset, bytes_to_copy);
        
        bytes_read += bytes_to_copy;
        size -= bytes_to_copy;
        cluster_offset = 0; // Only first cluster may have offset
        
        cluster = fat32_get_next_cluster(fs, cluster);
    }
    
    free(cluster_buffer);
    return bytes_read;
}

int fat32_write(vfs_node_t* node, uint64_t offset, uint64_t size, const void* buffer) {
    // Simplified write implementation - would need cluster allocation for full implementation
    fat32_node_data_t* node_data = (fat32_node_data_t*)node->fs_data;
    
    if (node_data->is_directory) {
        return VFS_ERROR_IS_DIR;
    }
    
    // For now, only support writing within existing file size
    if (offset + size > node->size) {
        return VFS_ERROR_NO_SPACE;
    }
    
    // Implementation would be similar to read but with write operations
    // and cluster allocation for extending files
    return VFS_ERROR_INVALID; // Not fully implemented
}

// --- Filename Conversion ---

void fat32_name_to_83(const char* name, char* name83) {
    memset(name83, ' ', 11);
    name83[11] = '\0';
    
    const char* dot = strrchr(name, '.');
    int name_len = dot ? (dot - name) : strlen(name);
    int ext_len = dot ? strlen(dot + 1) : 0;
    
    // Copy name part (up to 8 characters)
    for (int i = 0; i < name_len && i < 8; i++) {
        name83[i] = toupper(name[i]);
    }
    
    // Copy extension part (up to 3 characters)
    if (dot && ext_len > 0) {
        for (int i = 0; i < ext_len && i < 3; i++) {
            name83[8 + i] = toupper(dot[1 + i]);
        }
    }
}

void fat32_name_from_83(const char* name83, char* name) {
    int pos = 0;
    
    // Copy name part
    for (int i = 0; i < 8 && name83[i] != ' '; i++) {
        name[pos++] = tolower(name83[i]);
    }
    
    // Add extension if present
    if (name83[8] != ' ') {
        name[pos++] = '.';
        for (int i = 8; i < 11 && name83[i] != ' '; i++) {
            name[pos++] = tolower(name83[i]);
        }
    }
    
    name[pos] = '\0';
}

// --- Registration ---

int fat32_register(void) {
    return vfs_register_filesystem(&fat32_filesystem);
}
