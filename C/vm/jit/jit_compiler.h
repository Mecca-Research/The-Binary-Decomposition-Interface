
/**
 * @file jit_compiler.h
 * @brief JIT Compiler API
 * @details This file provides the jit compiler functionality for just-in-time compilation and optimization.
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef JIT_COMPILER_H
#define JIT_COMPILER_H

#include <stdint.h>
#include <stdbool.h>
#include "../bci_chunk.h"

// Forward declarations for LLVM types
typedef struct LLVMOpaqueContext *LLVMContextRef;
typedef struct LLVMOpaqueModule *LLVMModuleRef;
typedef struct LLVMOpaqueBuilder *LLVMBuilderRef;
typedef struct LLVMOpaqueExecutionEngine *LLVMExecutionEngineRef;
typedef struct LLVMOpaqueValue *LLVMValueRef;
typedef struct LLVMOpaqueType *LLVMTypeRef;

// JIT compilation tiers
typedef enum {
    JIT_TIER_INTERPRETER = 0,  // No JIT, pure interpretation
    JIT_TIER_BASELINE = 1,      // Fast compilation, minimal optimization
    JIT_TIER_OPTIMIZED = 2,     // Full optimization
    JIT_TIER_MAX = 3
} JITTier;

// JIT compilation status
typedef enum {
    JIT_STATUS_SUCCESS = 0,
    JIT_STATUS_ERROR_INIT = 1,
    JIT_STATUS_ERROR_COMPILE = 2,
    JIT_STATUS_ERROR_OPTIMIZE = 3,
    JIT_STATUS_ERROR_EXECUTE = 4
} JITStatus;

// Compiled function signature
typedef int64_t (*CompiledFunction)(void* context, int64_t* args, size_t arg_count);

// JIT compiler context
typedef struct {
    LLVMContextRef llvm_context;
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    LLVMExecutionEngineRef engine;
    
    // Compilation statistics
    uint64_t functions_compiled;
    uint64_t compilation_time_ns;
    uint64_t optimization_time_ns;
    
    // Configuration
    JITTier default_tier;
    bool enable_profiling;
    bool enable_inlining;
    uint32_t optimization_level;  // 0-3
} JITCompiler;

// Compiled code cache entry
typedef struct {
    uint32_t function_id;
    CompiledFunction native_code;
    JITTier tier;
    uint64_t execution_count;
    uint64_t total_time_ns;
    bool needs_recompilation;
} CompiledCode;

// JIT compiler API
JITCompiler* jit_compiler_create(void);
void jit_compiler_destroy(JITCompiler* compiler);

JITStatus jit_compiler_init(JITCompiler* compiler);
JITStatus jit_compiler_compile_function(
    JITCompiler* compiler,
    const Chunk* chunk,
    uint32_t function_id,
    JITTier tier,
    CompiledCode** out_code
);

JITStatus jit_compiler_optimize(JITCompiler* compiler, CompiledCode* code, JITTier new_tier);
JITStatus jit_compiler_execute(
    JITCompiler* compiler,
    CompiledCode* code,
    void* context,
    int64_t* args,
    size_t arg_count,
    int64_t* result
);

// Configuration
void jit_compiler_set_optimization_level(JITCompiler* compiler, uint32_t level);
void jit_compiler_enable_profiling(JITCompiler* compiler, bool enable);
void jit_compiler_enable_inlining(JITCompiler* compiler, bool enable);

// Statistics
void jit_compiler_get_stats(
    const JITCompiler* compiler,
    uint64_t* functions_compiled,
    uint64_t* compilation_time_ns,
    uint64_t* optimization_time_ns
);

void jit_compiler_reset_stats(JITCompiler* compiler);

// Code cache management
void compiled_code_destroy(CompiledCode* code);
bool compiled_code_should_recompile(const CompiledCode* code, uint64_t threshold);

#endif // JIT_COMPILER_H
