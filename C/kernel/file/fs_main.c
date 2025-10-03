// ===================================================================
// DESC: Implements the main file system logic (inodes, lookups, etc.).
// ===================================================================
#include "c23_compat.h"
#include "fs.h"
#include <stdio.h>

// Forward declarations for other layers
void log_init();
void bcache_init();
void log_commit();
void log_write();
[[nodiscard]] void* bcache_get(uint32_t blockno);

void fs_init() {
    log_init();
    bcache_init();
    printf("FS_MAIN: File system initialized.\n");
}

// Conceptual implementation of a file system read operation.
int fs_read(Inode* ip, char* dst, uint32_t off, uint32_t n) {
    printf("FS_MAIN: Reading %u bytes from inode %u at offset %u.\n",
           n, ip->type /* placeholder for inode number */, off);
           
    // Begin transaction
    // Loop through bytes to read:
    //   - Calculate block number
    //   - Get block from buffer cache (bcache_get)
    //   - Copy data from buffer to dst
    // End transaction
    log_commit();
    return n; // Return bytes read
}

// Conceptual implementation of a file system write operation.
int fs_write(Inode* ip, char* src, uint32_t off, uint32_t n) {
    printf("FS_MAIN: Writing %u bytes to inode %u at offset %u.\n",
           n, ip->type, off);

    // Begin transaction
    // Loop through bytes to write:
    //   - Calculate block number
    //   - Get block from buffer cache
    //   - Copy data from src to buffer
    //   - Mark buffer as dirty (log_write)
    // End transaction by committing the log
    log_commit();
    return n; // Return bytes written
}
