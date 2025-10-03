
/**
 * @file ext2.h
 * @brief ext2 filesystem with zero-copy I/O
 * 
 * Phase 5: Storage I/O Fast Paths
 */

#ifndef BDI_EXT2_H
#define BDI_EXT2_H

#include <stdint.h>
#include <stddef.h>

/* ext2 magic number */
#define EXT2_SUPER_MAGIC  0xEF53

/* ext2 inode modes */
#define EXT2_S_IFREG  0x8000
#define EXT2_S_IFDIR  0x4000

/* ext2 superblock */
struct ext2_superblock {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    uint8_t s_uuid[16];
    char s_volume_name[16];
    char s_last_mounted[64];
    uint32_t s_algorithm_usage_bitmap;
    uint8_t s_prealloc_blocks;
    uint8_t s_prealloc_dir_blocks;
    uint16_t s_padding1;
    uint8_t s_reserved[204];
} __attribute__((packed));

/* ext2 group descriptor */
struct ext2_group_desc {
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    uint16_t bg_pad;
    uint8_t bg_reserved[12];
} __attribute__((packed));

/* ext2 inode */
struct ext2_inode {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_block[15];
    uint32_t i_generation;
    uint32_t i_file_acl;
    uint32_t i_dir_acl;
    uint32_t i_faddr;
    uint8_t i_osd2[12];
} __attribute__((packed));

/* ext2 directory entry */
struct ext2_dirent {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[255];
} __attribute__((packed));

/* ext2 filesystem */
struct ext2_fs {
    struct ext2_superblock sb;
    struct ext2_group_desc *gdt;
    uint32_t block_size;
    uint32_t num_groups;
    void *device;
    int (*read_blocks)(void *dev, uint64_t block, void *buf, size_t count);
    int (*write_blocks)(void *dev, uint64_t block, const void *buf, size_t count);
};

/* ext2 file */
struct ext2_file {
    struct ext2_fs *fs;
    struct ext2_inode inode;
    uint32_t inode_num;
    uint32_t position;
};

/* Function prototypes */
int ext2_mount(struct ext2_fs *fs, void *device,
               int (*read_fn)(void *, uint64_t, void *, size_t),
               int (*write_fn)(void *, uint64_t, const void *, size_t));
int ext2_read_inode(struct ext2_fs *fs, uint32_t ino, struct ext2_inode *inode);
int ext2_open(struct ext2_fs *fs, const char *path, struct ext2_file *file);
int ext2_read(struct ext2_file *file, void *buf, size_t count);
int ext2_write(struct ext2_file *file, const void *buf, size_t count);
int ext2_close(struct ext2_file *file);
int ext2_readdir(struct ext2_fs *fs, uint32_t ino, struct ext2_dirent *entries, size_t max_entries);
void ext2_unmount(struct ext2_fs *fs);

#endif /* BDI_EXT2_H */
