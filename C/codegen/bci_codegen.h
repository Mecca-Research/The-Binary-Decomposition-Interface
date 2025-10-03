// ===================================================================
// DESC: Defines the Code Generator, which traverses the AST and emits
//       bytecode for the VM.
// ===================================================================
#ifndef BCI_CODEGEN_H
#define BCI_CODEGEN_H

#include "c23_compat.h"
#include "bci_ast.h"
#include "bci_chunk.h"
#include <stdbool.h>

// --- Code Generator Structure ---
// Holds the state for the code generation pass.
typedef struct {
    Chunk* compiling_chunk; // The chunk to write bytecode into.
    bool had_error;
} CodeGenerator;


// --- Code Generator Public API ---

// Initializes the code generator to write to the given chunk.
void codegen_init(CodeGenerator* codegen, Chunk* chunk);

// Frees any resources held by the code generator.
void codegen_free(CodeGenerator* codegen);

// The main entry point for code generation. Traverses the program's
// AST and emits the corresponding bytecode.
// Returns true on success, false if an error occurred.
bool codegen_generate(CodeGenerator* codegen, AstNode* program);



// Compile-time invariants
static_assert(sizeof(void*) >= 4, "Codegen requires at least 32-bit pointers");
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");

#endif // BCI_CODEGEN_H
