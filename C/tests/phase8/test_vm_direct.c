#include <stdio.h>
#include "../../vm/bci_vm.h"
#include "../../vm/bci_chunk.h"

int main() {
    printf("Testing VM result capture directly:\n\n");
    
    // Create a simple chunk: CONSTANT 42, RETURN
    Chunk chunk;
    chunk_init(&chunk);
    
    // Add constant 42.0
    int const_idx = chunk_add_constant(&chunk, 42.0);
    
    // Emit: CONSTANT const_idx
    chunk_write(&chunk, OP_CONSTANT, 0);
    chunk_write(&chunk, (uint8_t)const_idx, 0);
    
    // Emit: RETURN
    chunk_write(&chunk, OP_RETURN, 0);
    
    printf("Chunk created with %d bytes\n", chunk.count);
    printf("Bytecode: [%d, %d, %d]\n", chunk.code[0], chunk.code[1], chunk.code[2]);
    printf("Constant[0] = %.6f\n\n", chunk.constants.data[0]);
    
    // Create VM and execute
    VM vm;
    vm_init(&vm);
    
    printf("Executing with vm_interpret_with_result...\n");
    BciVmResult result = vm_interpret_with_result(&vm, &chunk);
    
    printf("\nResult:\n");
    printf("  status = %d (0=OK)\n", result.status);
    printf("  result_value = %.6f\n", result.result_value);
    
    // Check stack state
    printf("\nVM stack state:\n");
    printf("  stack_top - stack = %ld\n", (long)(vm.stack_top - vm.stack));
    
    vm_free(&vm);
    chunk_free(&chunk);
    
    return 0;
}
