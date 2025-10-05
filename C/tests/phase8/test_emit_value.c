#include <stdio.h>
#include "../../compiler/codegen/codegen.h"
#include "../../vm/bci_chunk.h"

int main() {
    printf("Testing opcode values:\n");
    printf("OP_RETURN = %d\n", OP_RETURN);
    printf("OPCODE_RETURN = %d\n", OPCODE_RETURN);
    printf("OPCODE_NOP = %d\n", OPCODE_NOP);
    
    // Create a chunk and emit OPCODE_RETURN
    Chunk chunk;
    chunk_init(&chunk);
    
    CodeGenerator* codegen = codegen_create();
    codegen->current_function = function_context_create(NULL, "test");
    codegen->current_function->chunk = &chunk;
    
    printf("\nEmitting OPCODE_RETURN...\n");
    codegen_emit_byte(codegen, OPCODE_RETURN);
    
    printf("Chunk size: %d\n", chunk.count);
    if (chunk.count > 0) {
        printf("Emitted byte: %d\n", chunk.code[0]);
    }
    
    chunk_free(&chunk);
    codegen_destroy(codegen);
    
    return 0;
}
