/*
 * OPCODE COMPATIBILITY NOTE:
 * 
 * This harness has been updated to work with the current VM implementation
 * which supports only 7 basic opcodes (defined in C/vm/bci_chunk.h).
 * 
 * Graph execution features are not yet available in the current VM.
 * This is a placeholder that will be activated when graph APIs are stable.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "vm/bci_vm.h"
#include "vm/bci_chunk.h"

// TODO: Implement graph execution fuzzing when APIs are available
// For now, this is a minimal placeholder that tests basic VM execution

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1 || size > 1024) return 0;
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Create simple bytecode from fuzz input
    for (size_t i = 0; i < size && i < 256; i++) {
        if (data[i] <= OP_DIVIDE) {
            chunk_write(&chunk, data[i], i);
        }
    }
    
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
