
// ===================================================================
// DESC: BDI Pipeline Implementation
// ===================================================================

#include "bdi_pipeline.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Get current time in microseconds
static uint64_t get_time_us(void) {
    // Simple fallback using clock() if clock_gettime not available
    return (uint64_t)clock() * 1000000 / CLOCKS_PER_SEC;
}

// Create default configuration
PipelineConfig pipeline_default_config(void) {
    PipelineConfig config;
    config.enable_optimization = true;
    config.enable_debug_info = true;
    config.enable_gc = true;
    config.enable_jit = false;
    config.verbose = false;
    return config;
}

// Create pipeline with default config
PipelineContext* pipeline_create(void) {
    return pipeline_create_with_config(pipeline_default_config());
}

// Create pipeline with custom config
PipelineContext* pipeline_create_with_config(PipelineConfig config) {
    PipelineContext* ctx = (PipelineContext*)malloc(sizeof(PipelineContext));
    if (!ctx) return NULL;
    
    // Initialize all fields to NULL/0
    memset(ctx, 0, sizeof(PipelineContext));
    
    // Set configuration
    ctx->config = config;
    
    // Create code generator
    ctx->codegen = codegen_create();
    if (!ctx->codegen) {
        free(ctx);
        return NULL;
    }
    
    // Configure code generator
    ctx->codegen->optimize = config.enable_optimization;
    ctx->codegen->debug_info = config.enable_debug_info;
    
    // Create chunk
    ctx->chunk = (Chunk*)malloc(sizeof(Chunk));
    if (!ctx->chunk) {
        codegen_destroy(ctx->codegen);
        free(ctx);
        return NULL;
    }
    chunk_init(ctx->chunk);
    
    // Create VM (basic or enhanced based on config)
    if (config.enable_gc) {
        ctx->enhanced_vm = enhanced_vm_create(10 * 1024 * 1024); // 10MB heap
        if (!ctx->enhanced_vm) {
            chunk_free(ctx->chunk);
            free(ctx->chunk);
            codegen_destroy(ctx->codegen);
            free(ctx);
            return NULL;
        }
        ctx->vm = ctx->enhanced_vm->base_vm;
    } else {
        ctx->vm = (VM*)malloc(sizeof(VM));
        if (!ctx->vm) {
            chunk_free(ctx->chunk);
            free(ctx->chunk);
            codegen_destroy(ctx->codegen);
            free(ctx);
            return NULL;
        }
        vm_init(ctx->vm);
    }
    
    // Initialize result
    ctx->result.success = false;
    ctx->result.result_value = 0.0;
    ctx->result.error_message = NULL;
    ctx->result.error_line = 0;
    ctx->result.compile_time_us = 0;
    ctx->result.execute_time_us = 0;
    ctx->result.bytecode_size = 0;
    ctx->result.constants_count = 0;
    
    return ctx;
}

// Destroy pipeline
void pipeline_destroy(PipelineContext* ctx) {
    if (!ctx) return;
    
    // Free enhanced VM (which includes base VM)
    if (ctx->enhanced_vm) {
        enhanced_vm_destroy(ctx->enhanced_vm);
    } else if (ctx->vm) {
        vm_free(ctx->vm);
        free(ctx->vm);
    }
    
    // Free chunk
    if (ctx->chunk) {
        chunk_free(ctx->chunk);
        free(ctx->chunk);
    }
    
    // Free code generator
    if (ctx->codegen) {
        codegen_destroy(ctx->codegen);
    }
    
    free(ctx);
}

// Compile source code
bool pipeline_compile(PipelineContext* ctx, const char* source) {
    if (!ctx || !source) return false;
    
    uint64_t start_time = get_time_us();
    
    if (ctx->config.verbose) {
        printf("=== Compilation Started ===\n");
        printf("Source: %s\n", source);
    }
    
    // Step 1: Lexical Analysis
    if (ctx->config.verbose) printf("Step 1: Lexical Analysis...\n");
    lexer_init(&ctx->lexer, source);
    
    // Step 2: Parsing
    if (ctx->config.verbose) printf("Step 2: Parsing...\n");
    parser_init(&ctx->parser, &ctx->lexer);
    AstNode* ast = parser_parse(&ctx->parser);
    
    if (!ast || ctx->parser.had_error) {
        ctx->result.success = false;
        ctx->result.error_message = "Parse error";
        ctx->result.error_line = ctx->parser.current.line;
        if (ctx->config.verbose) printf("Parse error at line %d\n", ctx->result.error_line);
        return false;
    }
    
    if (ctx->config.verbose) printf("Step 3: Code Generation...\n");
    
    // Step 3: Code Generation
    // Generate bytecode from AST (codegen_generate takes AST and Chunk)
    bool codegen_success = codegen_generate(ctx->codegen, ast, ctx->chunk);
    
    // Free AST
    ast_free_node(ast);
    
    if (!codegen_success || ctx->codegen->had_error) {
        ctx->result.success = false;
        ctx->result.error_message = ctx->codegen->error_message ? 
                                    ctx->codegen->error_message : "Code generation error";
        ctx->result.error_line = ctx->codegen->error_line;
        if (ctx->config.verbose) {
            printf("Code generation error: %s (line %d)\n", 
                   ctx->result.error_message, ctx->result.error_line);
        }
        return false;
    }
    
    // Step 4: Optimization (if enabled)
    if (ctx->config.enable_optimization) {
        if (ctx->config.verbose) printf("Step 4: Optimization...\n");
        codegen_optimize_chunk(ctx->chunk);
    }
    
    // Record compilation statistics
    ctx->result.compile_time_us = get_time_us() - start_time;
    ctx->result.bytecode_size = ctx->chunk->count;
    ctx->result.constants_count = ctx->chunk->constants.len;
    
    if (ctx->config.verbose) {
        printf("=== Compilation Complete ===\n");
        printf("Bytecode size: %zu bytes\n", ctx->result.bytecode_size);
        printf("Constants: %zu\n", ctx->result.constants_count);
        printf("Compile time: %lu us\n", ctx->result.compile_time_us);
    }
    
    ctx->result.success = true;
    return true;
}

// Execute compiled bytecode
bool pipeline_execute(PipelineContext* ctx) {
    if (!ctx || !ctx->result.success) return false;
    
    uint64_t start_time = get_time_us();
    
    if (ctx->config.verbose) {
        printf("\n=== Execution Started ===\n");
    }
    
    // Execute bytecode
    BciVmResult vm_result;
    if (ctx->enhanced_vm) {
        // Enhanced VM doesn't use BciVmResult yet, use legacy path
        InterpretResult legacy_result = enhanced_vm_execute(ctx->enhanced_vm, ctx->chunk) ? 
                                        INTERPRET_OK : INTERPRET_RUNTIME_ERROR;
        vm_result.status = legacy_result;
        // For enhanced VM, try to get result from stack if available
        if (ctx->vm->stack_top > ctx->vm->stack) {
            vm_result.result_value = ctx->vm->stack[0];
        } else {
            vm_result.result_value = 0.0;
        }
    } else {
        // Use new result-capturing VM function
        vm_result = vm_interpret_with_result(ctx->vm, ctx->chunk);
    }
    
    ctx->result.execute_time_us = get_time_us() - start_time;
    
    if (vm_result.status != INTERPRET_OK) {
        ctx->result.success = false;
        ctx->result.error_message = "Runtime error";
        if (ctx->config.verbose) {
            printf("Runtime error during execution\n");
        }
        return false;
    }
    
    // Get result value from VM result structure
    ctx->result.result_value = vm_result.result_value;
    
    if (ctx->config.verbose) {
        printf("=== Execution Complete ===\n");
        printf("Result: %.6f\n", ctx->result.result_value);
        printf("Execute time: %lu us\n", ctx->result.execute_time_us);
    }
    
    return true;
}

// Compile and execute in one step
PipelineResult pipeline_run(const char* source) {
    return pipeline_run_with_config(source, pipeline_default_config());
}

// Compile and execute with custom config
PipelineResult pipeline_run_with_config(const char* source, PipelineConfig config) {
    PipelineContext* ctx = pipeline_create_with_config(config);
    if (!ctx) {
        PipelineResult result;
        result.success = false;
        result.error_message = "Failed to create pipeline";
        result.error_line = 0;
        result.result_value = 0.0;
        result.compile_time_us = 0;
        result.execute_time_us = 0;
        result.bytecode_size = 0;
        result.constants_count = 0;
        return result;
    }
    
    // Compile
    if (!pipeline_compile(ctx, source)) {
        PipelineResult result = ctx->result;
        pipeline_destroy(ctx);
        return result;
    }
    
    // Execute
    if (!pipeline_execute(ctx)) {
        PipelineResult result = ctx->result;
        pipeline_destroy(ctx);
        return result;
    }
    
    PipelineResult result = ctx->result;
    pipeline_destroy(ctx);
    return result;
}

// Get result
PipelineResult pipeline_get_result(const PipelineContext* ctx) {
    if (!ctx) {
        PipelineResult result;
        result.success = false;
        result.error_message = "Invalid context";
        result.error_line = 0;
        result.result_value = 0.0;
        result.compile_time_us = 0;
        result.execute_time_us = 0;
        result.bytecode_size = 0;
        result.constants_count = 0;
        return result;
    }
    return ctx->result;
}

// Print statistics
void pipeline_print_stats(const PipelineContext* ctx) {
    if (!ctx) return;
    
    printf("\n========================================\n");
    printf("Pipeline Statistics\n");
    printf("========================================\n");
    printf("Status:           %s\n", ctx->result.success ? "SUCCESS" : "FAILED");
    
    if (!ctx->result.success && ctx->result.error_message) {
        printf("Error:            %s (line %d)\n", 
               ctx->result.error_message, ctx->result.error_line);
    }
    
    if (ctx->result.success) {
        printf("Result:           %.6f\n", ctx->result.result_value);
    }
    
    printf("Compile time:     %lu us\n", ctx->result.compile_time_us);
    printf("Execute time:     %lu us\n", ctx->result.execute_time_us);
    printf("Total time:       %lu us\n", 
           ctx->result.compile_time_us + ctx->result.execute_time_us);
    printf("Bytecode size:    %zu bytes\n", ctx->result.bytecode_size);
    printf("Constants:        %zu\n", ctx->result.constants_count);
    
    if (ctx->enhanced_vm) {
        uint64_t gc_collections, gc_allocated, gc_freed;
        size_t young_used, old_used;
        enhanced_vm_get_gc_stats(ctx->enhanced_vm, &gc_collections, 
                                &gc_allocated, &gc_freed, &young_used, &old_used);
        printf("GC collections:   %lu\n", gc_collections);
        printf("GC allocated:     %lu bytes\n", gc_allocated);
        printf("GC freed:         %lu bytes\n", gc_freed);
        printf("Young gen used:   %zu bytes\n", young_used);
        printf("Old gen used:     %zu bytes\n", old_used);
    }
    
    printf("========================================\n");
}

// Disassemble bytecode
void pipeline_disassemble(const PipelineContext* ctx) {
    if (!ctx || !ctx->chunk) return;
    
    printf("\n========================================\n");
    printf("Bytecode Disassembly\n");
    printf("========================================\n");
    
    codegen_disassemble_chunk(ctx->chunk, "<main>");
    
    printf("========================================\n");
}

// Validate source code
bool pipeline_validate(const char* source, const char** error_message) {
    PipelineContext* ctx = pipeline_create();
    if (!ctx) {
        if (error_message) *error_message = "Failed to create pipeline";
        return false;
    }
    
    bool success = pipeline_compile(ctx, source);
    
    if (!success && error_message) {
        *error_message = ctx->result.error_message;
    }
    
    pipeline_destroy(ctx);
    return success;
}

// Get bytecode
Chunk* pipeline_get_bytecode(PipelineContext* ctx) {
    return ctx ? ctx->chunk : NULL;
}

// Reset pipeline
void pipeline_reset(PipelineContext* ctx) {
    if (!ctx) return;
    
    // Reset chunk
    if (ctx->chunk) {
        chunk_free(ctx->chunk);
        chunk_init(ctx->chunk);
    }
    
    // Reset VM
    if (ctx->vm) {
        vm_reset(ctx->vm);
    }
    
    // Reset code generator
    if (ctx->codegen) {
        ctx->codegen->had_error = false;
        ctx->codegen->error_message = NULL;
        ctx->codegen->error_line = 0;
    }
    
    // Reset result
    ctx->result.success = false;
    ctx->result.result_value = 0.0;
    ctx->result.error_message = NULL;
    ctx->result.error_line = 0;
    ctx->result.compile_time_us = 0;
    ctx->result.execute_time_us = 0;
    ctx->result.bytecode_size = 0;
    ctx->result.constants_count = 0;
}
