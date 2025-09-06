// ===================================================================
// DESC: Implementation of the Chunk for storing bytecode.
// ===================================================================

#include "bci_chunk.h"
#include <stdlib.h>

// --- Chunk Public API Implementation ---

void chunk_init(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    bci_vec_init(&chunk->constants);
}

void chunk_free(Chunk* chunk) {
    free(chunk->code);
    free(chunk->lines);
    bci_vec_free(&chunk->constants);
    chunk_init(chunk); // Reset to a clean state
}

void chunk_write(Chunk* chunk, uint8_t byte, int line) {
    if (chunk->capacity < chunk->count + 1) {
        int old_capacity = chunk->capacity;
        chunk->capacity = old_capacity < 8 ? 8 : old_capacity * 2;
        chunk->code = realloc(chunk->code, chunk->capacity * sizeof(uint8_t));
        chunk->lines = realloc(chunk->lines, chunk->capacity * sizeof(int));
        // Note: In a real-world application, check if realloc returned NULL
    }

    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

int chunk_add_constant(Chunk* chunk, double value) {
    bci_vec_push(&chunk->constants, value);
    return chunk->constants.len - 1;
}
