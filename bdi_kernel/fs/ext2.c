
/**
 * @file ext2.c
 * @brief ext2 filesystem implementation
 * 
 * Phase 5: Storage I/O Fast Paths
 */

#include "ext2.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

[[maybe_unused]] static int ext2_debug = 0;

/**
 * @brief Mount ext2 filesystem
 */
int ext2_mount(struct ext2_fs *fs, void *device,
               int (*read_fn)(void *, uint64_t, void *, size_t),
               int (*write_fn)(void *, uint64_t, const void *, size_t)) {
    int ret;
    
    if (!fs || !device || !read_fn) {
        return -EINVAL;
    }
    
    memset(fs, 0, sizeof(*fs));
    fs->device = device;
    fs->read_blocks = read_fn;
    fs->write_blocks = write_fn;
    
    /* Read superblock (at block 1, offset 1024) */
    ret = read_fn(device, 2, &fs->sb, 2); /* 2 sectors = 1024 bytes */
    if (ret < 0) {
        return ret;
    }
    
    /* Validate ext2 */
    if (fs->sb.s_magic != EXT2_SUPER_MAGIC) {
        return -EINVAL;
    }
    
    /* Calculate block size */
    fs->block_size = 1024 << fs->sb.s_log_block_size;
    
    /* Calculate number of block groups */
    fs->num_groups = (fs->sb.s_blocks_count + fs->sb.s_blocks_per_group - 1) / 
                     fs->sb.s_blocks_per_group;
    
    /* Allocate and read group descriptor table */
    size_t gdt_size = fs->num_groups * sizeof(struct ext2_group_desc);
    fs->gdt = malloc(gdt_size);
    if (!fs->gdt) {
        return -ENOMEM;
    }
    
    /* GDT starts at block after superblock */
    uint64_t gdt_block = (fs->block_size == 1024) ? 2 : 1;
    ret = read_fn(device, gdt_block * (fs->block_size / 512), fs->gdt, 
                  (gdt_size + 511) / 512);
    if (ret < 0) {
        free(fs->gdt);
        return ret;
    }
    
    return 0;
}

/**
 * @brief Read inode
 */
int ext2_read_inode(struct ext2_fs *fs, uint32_t ino, struct ext2_inode *inode) {
    uint32_t group, index;
    struct ext2_group_desc *gd;
    uint64_t inode_table_block;
    
    if (!fs || !inode || ino == 0) {
        return -EINVAL;
    }
    
    /* Calculate group and index */
    group = (ino - 1) / fs->sb.s_inodes_per_group;
    index = (ino - 1) % fs->sb.s_inodes_per_group;
    
    if (group >= fs->num_groups) {
        return -EINVAL;
    }
    
    gd = &fs->gdt[group];
    inode_table_block = gd->bg_inode_table;
    
    /* Calculate inode offset */
    uint32_t inode_size = (fs->sb.s_rev_level == 0) ? 128 : fs->sb.s_inode_size;
    uint64_t offset = inode_table_block * fs->block_size + index * inode_size;
    uint64_t sector = offset / 512;
    
    /* Read inode */
    uint8_t buf[512];
    int ret = fs->read_blocks(fs->device, sector, buf, 1);
    if (ret < 0) {
        return ret;
    }
    
    memcpy(inode, buf + (offset % 512), sizeof(*inode));
    return 0;
}

/**
 * @brief Open file
 */
int ext2_open(struct ext2_fs *fs, const char *path, struct ext2_file *file) {
    /* TODO: Parse path and find inode */
    /* TODO: Read directory entries */
    
    if (!fs || !path || !file) {
        return -EINVAL;
    }
    
    memset(file, 0, sizeof(*file));
    file->fs = fs;
    file->inode_num = 2; /* Root inode */
    
    return ext2_read_inode(fs, file->inode_num, &file->inode);
}

/**
 * @brief Read from file
 */
int ext2_read(struct ext2_file *file, void *buf, size_t count) {
    uint32_t block_offset, bytes_read = 0;
    uint8_t *buffer = buf;
    
    if (!file || !buf) {
        return -EINVAL;
    }
    
    while (count > 0 && file->position < file->inode.i_size) {
        uint32_t block_num = file->position / file->fs->block_size;
        block_offset = file->position % file->fs->block_size;
        
        /* Get block number from inode */
        if (block_num >= 12) {
            /* TODO: Handle indirect blocks */
            return -ENOSYS;
        }
        
        uint32_t phys_block = file->inode.i_block[block_num];
        if (phys_block == 0) {
            break; /* Sparse file */
        }
        
        /* Read block */
        uint8_t block_buf[4096];
        int ret = file->fs->read_blocks(file->fs->device, 
                                       phys_block * (file->fs->block_size / 512),
                                       block_buf,
                                       file->fs->block_size / 512);
        if (ret < 0) {
            return ret;
        }
        
        /* Copy data */
        uint32_t to_copy = file->fs->block_size - block_offset;
        if (to_copy > count) {
            to_copy = count;
        }
        if (file->position + to_copy > file->inode.i_size) {
            to_copy = file->inode.i_size - file->position;
        }
        
        memcpy(buffer + bytes_read, block_buf + block_offset, to_copy);
        bytes_read += to_copy;
        count -= to_copy;
        file->position += to_copy;
    }
    
    return bytes_read;
}

/**
 * @brief Write to file
 */
int ext2_write(struct ext2_file *file, const void *buf, size_t count) {
    /* TODO: Implement write */
    return -ENOSYS;
}

/**
 * @brief Close file
 */
int ext2_close(struct ext2_file *file) {
    if (!file) {
        return -EINVAL;
    }
    
    memset(file, 0, sizeof(*file));
    return 0;
}

/**
 * @brief Read directory
 */
int ext2_readdir(struct ext2_fs *fs, uint32_t ino, struct ext2_dirent *entries, size_t max_entries) {
    /* TODO: Implement readdir */
    return -ENOSYS;
}

/**
 * @brief Unmount filesystem
 */
void ext2_unmount(struct ext2_fs *fs) {
    if (!fs) {
        return;
    }
    
    free(fs->gdt);
    memset(fs, 0, sizeof(*fs));
}
