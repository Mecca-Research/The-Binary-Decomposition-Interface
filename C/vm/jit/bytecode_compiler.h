
/**
 * @file bytecode_compiler.h
 * @brief Compiler Infrastructure
 * @details This file provides the bytecode compiler functionality for just-in-time compilation and optimization.
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef BYTECODE_COMPILER_H
#define BYTECODE_COMPILER_H

#include <stdint.h>
#include <stdbool.h>
#include "../bci_chunk.h"
#include "jit_compiler.h"

// Bytecode to IR compilation context
typedef struct {
    LLVMContextRef llvm_context;
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    
    // Current function being compiled
    LLVMValueRef current_function;
    LLVMValueRef* local_vars;
    size_t local_count;
    
    // Basic blocks
    LLVMValueRef* basic_blocks;
    size_t block_count;
    
    // Compilation options
    bool enable_bounds_checking;
    bool enable_overflow_checking;
    bool enable_debug_info;
} BytecodeCompiler;

// Compilation result
typedef struct {
    LLVMValueRef function;
    uint32_t instruction_count;
    uint32_t basic_block_count;
    bool has_loops;
    bool has_calls;
} CompilationResult;

// Bytecode compiler API
BytecodeCompiler* bytecode_compiler_create(LLVMContextRef context, LLVMModuleRef module);
void bytecode_compiler_destroy(BytecodeCompiler* compiler);

bool bytecode_compiler_compile_chunk(
    BytecodeCompiler* compiler,
    const Chunk* chunk,
    uint32_t function_id,
    CompilationResult* result
);

bool bytecode_compiler_compile_instruction(
    BytecodeCompiler* compiler,
    uint8_t opcode,
    const uint8_t* operands,
    size_t operand_count
);

// IR optimization passes
bool bytecode_compiler_optimize_ir(BytecodeCompiler* compiler, LLVMValueRef function);
bool bytecode_compiler_inline_functions(BytecodeCompiler* compiler, LLVMValueRef function);
bool bytecode_compiler_eliminate_dead_code(BytecodeCompiler* compiler, LLVMValueRef function);

// Configuration
void bytecode_compiler_enable_bounds_checking(BytecodeCompiler* compiler, bool enable);
void bytecode_compiler_enable_overflow_checking(BytecodeCompiler* compiler, bool enable);
void bytecode_compiler_enable_debug_info(BytecodeCompiler* compiler, bool enable);

#endif // BYTECODE_COMPILER_H
