// ===================================================================
// DESC: Defines the on-disk data structures for the Aeon file system.
// ===================================================================
#ifndef AEON_FS_H
#define AEON_FS_H

#include <stdint.h>

#define FS_BLOCK_SIZE 4096
#define MAX_FILENAME 28
#define INODE_DIRECT_BLOCKS 12

// --- On-Disk Structures ---

typedef struct {
    uint32_t size;         // Size of file system in blocks
    uint32_t log_start;    // Block number of first log block
    uint32_t inode_start;  // Block number of first inode block
    uint32_t bmap_start;   // Block number of first bitmap block
} Superblock;

typedef struct {
    uint16_t type;         // File type (e.g., file, directory)
    uint16_t major, minor; // Device numbers (if special file)
    uint32_t size;         // Size of file in bytes
    uint32_t addrs[INODE_DIRECT_BLOCKS + 1]; // Data block addresses
} Inode;

typedef struct {
    uint32_t inum;
    char name[MAX_FILENAME];
} DirectoryEntry;

#endif // AEON_FS_H
