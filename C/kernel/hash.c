// ===================================================================
// DESC: Provides a simple hashing function for metadata.
//       A real implementation would use a robust algorithm like SHA-256.
// ===================================================================
#include "graph.h" // For NodeMeta struct
#include <string.h>

// Simple djb2 hash algorithm for demonstration.
void aeon_hash_meta(const GraphNode* node, uint8_t out_hash[32]) {
    unsigned long hash = 5381;
    const unsigned char* data = (const unsigned char*)node;
    // Hash the core, immutable properties of the node
    for (size_t i = 0; i < sizeof(GraphNode); ++i) {
        hash = ((hash << 5) + hash) + data[i];
    }
    // Clear the output and copy the hash value
    memset(out_hash, 0, 32);
    memcpy(out_hash, &hash, sizeof(hash));
}
