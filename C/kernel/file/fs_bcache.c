// ===================================================================
// DESC: Implements the buffer cache layer to keep frequently used
//       disk blocks in HAM regions.
// ===================================================================
#include "fs.h"
#include "ham.h"
#include <stdio.h>

void bcache_init() {
    // Initialize the list of cached buffers.
    printf("FS_BCACHE: Buffer cache initialized.\n");
}

// Gets a block from the cache. If not present, allocates a HAM
// region and reads it from the archive (disk).
void* bcache_get(uint32_t blockno) {
    printf("FS_BCACHE: Requesting block %u.\n", blockno);
    // 1. Search for block in the cache.
    // 2. If not found:
    //    a. Allocate a HAM_ACTIVE region.
    //    b. ham->load(...) the block from disk.
    //    c. Add to cache.
    // 3. Return pointer to the cached data.
    return NULL; // Placeholder
}
