/*
 * OPCODE COMPATIBILITY NOTE:
 * 
 * This harness has been updated to work with the current VM implementation
 * which supports only 7 basic opcodes (defined in C/vm/bci_chunk.h):
 *   - OP_RETURN      (return from function)
 *   - OP_CONSTANT    (load constant)
 *   - OP_NEGATE      (unary negation)
 *   - OP_ADD         (addition)
 *   - OP_SUBTRACT    (subtraction)
 *   - OP_MULTIPLY    (multiplication)
 *   - OP_DIVIDE      (division)
 * 
 * This is a minimal version that tests bytecode parsing with available opcodes.
 * See C/fuzzing/OPCODE_ANALYSIS.md for detailed opcode usage analysis.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "vm/bci_vm.h"
#include "vm/bci_chunk.h"

// Test bytecode parsing and validation
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1 || size > 2048) return 0;
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Parse fuzzed bytecode
    for (size_t i = 0; i < size; i++) {
        uint8_t opcode = data[i];
        
        // Validate opcode is in valid range
        if (opcode <= OP_DIVIDE) {
            chunk_write(&chunk, opcode, i);
            
            // Handle opcodes with operands
            if (opcode == OP_CONSTANT && i + 1 < size) {
                // Add constant index
                i++;
                chunk_write(&chunk, data[i], i);
            }
        }
    }
    
    chunk_free(&chunk);
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
