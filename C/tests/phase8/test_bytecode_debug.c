#include <stdio.h>
#include "../../bdi_pipeline.h"
#include "../../vm/bci_chunk.h"

int main(void) {
    printf("Testing bytecode generation for: 42;\n\n");
    
    PipelineContext* ctx = pipeline_create();
    if (!ctx) {
        printf("Failed to create pipeline\n");
        return 1;
    }
    
    if (!pipeline_compile(ctx, "42;")) {
        printf("Compilation failed: %s\n", ctx->result.error_message);
        pipeline_destroy(ctx);
        return 1;
    }
    
    printf("Compilation successful!\n");
    printf("Bytecode size: %zu bytes\n", ctx->chunk->count);
    printf("Constants: %zu\n", ctx->chunk->constants.len);
    
    printf("\nBytecode:\n");
    for (size_t i = 0; i < ctx->chunk->count; i++) {
        printf("  [%zu] = %d (0x%02x)\n", i, ctx->chunk->code[i], ctx->chunk->code[i]);
    }
    
    printf("\nConstants:\n");
    for (size_t i = 0; i < ctx->chunk->constants.len; i++) {
        printf("  [%zu] = %.6f\n", i, ctx->chunk->constants.data[i]);
    }
    
    printf("\nExpected opcodes:\n");
    printf("  OP_CONSTANT = %d\n", OP_CONSTANT);
    printf("  OP_RETURN = %d\n", OP_RETURN);
    
    pipeline_destroy(ctx);
    return 0;
}
