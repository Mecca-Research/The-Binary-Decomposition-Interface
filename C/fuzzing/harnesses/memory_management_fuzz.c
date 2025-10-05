/*
 * OPCODE COMPATIBILITY NOTE:
 * 
 * This harness has been updated to work with the current VM implementation
 * which supports only 7 basic opcodes (defined in C/vm/bci_chunk.h).
 * 
 * Advanced memory management features (GC, reallocate) are not yet exposed.
 * This tests basic memory operations available in the current VM.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "vm/bci_vm.h"
#include "vm/bci_chunk.h"

// Test memory management through VM operations
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1 || size > 2048) return 0;
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Create bytecode that stresses memory (constants, stack ops)
    for (size_t i = 0; i < size && i < 512; i++) {
        uint8_t op = data[i] % 8;
        
        if (op == OP_CONSTANT && i + 1 < size) {
            // Add constant (tests constant pool memory)
            double value = (double)data[i + 1];
            int constant_idx = chunk_add_constant(&chunk, value);
            chunk_write(&chunk, OP_CONSTANT, i);
            chunk_write(&chunk, (uint8_t)constant_idx, i);
            i++;
        } else if (op <= OP_DIVIDE) {
            chunk_write(&chunk, op, i);
        }
    }
    
    // Terminate bytecode with OP_RETURN to prevent reading uninitialized memory
    chunk_write(&chunk, OP_RETURN, size);

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
