/*
 * OPCODE COMPATIBILITY NOTE:
 * 
 * This harness has been updated to work with the current VM implementation
 * which supports only 7 basic opcodes (defined in C/vm/bci_chunk.h).
 * 
 * The current VM uses doubles for values. This tests value operations.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "vm/bci_vm.h"
#include "vm/bci_chunk.h"

// Test value system (currently doubles)
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 8 || size > 2048) return 0;
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Create bytecode that tests value operations
    size_t i = 0;
    int constant_count = 0;
    
    while (i + 8 <= size && constant_count < 256) {
        // Extract double value from fuzz data
        double value;
        memcpy(&value, &data[i], sizeof(double));
        i += 8;
        
        // Test edge cases
        if (isnan(value) || isinf(value)) {
            // Skip problematic values
            continue;
        }
        
        // Add constant and operations
        int idx = chunk_add_constant(&chunk, value);
        chunk_write(&chunk, OP_CONSTANT, 0);
        chunk_write(&chunk, (uint8_t)idx, 0);
        constant_count++;
        
        // Add arithmetic operation if we have more data
        if (i < size) {
            uint8_t op_offset = data[i++] % 5;
            uint8_t op = OP_NEGATE + op_offset;  // NEGATE through DIVIDE
            if (op <= OP_DIVIDE) {
                chunk_write(&chunk, op, 0);
            }
        }
    }
    
    chunk_write(&chunk, OP_RETURN, 0);
    
    vm_interpret(&vm, &chunk);
    
    chunk_free(&chunk);
    vm_free(&vm);
    return 0;
}

#ifdef __AFL_COMPILER
int main(int argc, char** argv) {
    if (argc != 2) return 1;
    
    FILE* f = fopen(argv[1], "rb");
    if (!f) return 1;
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t* data = malloc(size);
    fread(data, 1, size, f);
    fclose(f);
    
    LLVMFuzzerTestOneInput(data, size);
    free(data);
    return 0;
}
#endif
