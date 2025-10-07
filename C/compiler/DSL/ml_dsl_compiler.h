
/**
 * @file ml_dsl_compiler.h
 * @brief ML DSL Compiler - Semantic Analysis and Code Generation
 * @details Compiles ML DSL AST to VM bytecode
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * 
 * @author BDI Kernel Team
 * @date 2025
 */
#ifndef BDI_ML_DSL_COMPILER_H
#define BDI_ML_DSL_COMPILER_H

#include "ml_dsl_ast.h"
#include "../CodeGen/ml_codegen.h"
#include "../VM/ml_ops.h"
#include "../../c23_compat.h"
#include <stddef.h>
#include <stdbool.h>

// Compiler context
typedef struct {
    // Symbol table for models
    struct {
        char** names;
        void** models;
        size_t count;
        size_t capacity;
    } symbol_table;
    
    // Error tracking
    bool has_error;
    char* error_message;
    
    // Code generation
    MLVMContext* vm_context;
} DSLCompiler;

// Compiler lifecycle
DSLCompiler* dsl_compiler_create(void);
void dsl_compiler_destroy(DSLCompiler* compiler);

// Compilation
bool dsl_compiler_compile(DSLCompiler* compiler, ASTNode* ast);
bool dsl_compiler_compile_model_decl(DSLCompiler* compiler, ASTNode* node);
bool dsl_compiler_compile_train_stmt(DSLCompiler* compiler, ASTNode* node);
bool dsl_compiler_compile_predict_stmt(DSLCompiler* compiler, ASTNode* node);

// Semantic analysis
bool dsl_compiler_validate_model_decl(DSLCompiler* compiler, ASTNode* node);
bool dsl_compiler_validate_train_stmt(DSLCompiler* compiler, ASTNode* node);
bool dsl_compiler_validate_predict_stmt(DSLCompiler* compiler, ASTNode* node);

// Symbol table
void dsl_compiler_add_model(DSLCompiler* compiler, const char* name, void* model);
void* dsl_compiler_get_model(DSLCompiler* compiler, const char* name);
bool dsl_compiler_has_model(DSLCompiler* compiler, const char* name);

// Error handling
bool dsl_compiler_has_error(const DSLCompiler* compiler);
const char* dsl_compiler_get_error(const DSLCompiler* compiler);
void dsl_compiler_report_error(DSLCompiler* compiler, const char* message);

// High-level API
bool dsl_compile_source(const char* source, MLVMContext** out_vm_context);
bool dsl_compile_file(const char* filename, MLVMContext** out_vm_context);

#endif // BDI_ML_DSL_COMPILER_H
