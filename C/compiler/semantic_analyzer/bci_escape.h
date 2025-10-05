
// ===================================================================
// DESC: Escape Analysis for Phase 3.3
//       Determines if values escape to heap, return, or global scope
// ===================================================================
/**
 * @file bci_escape.h
 * @brief Bci Escape API
 * @details This file provides the bci escape functionality for the BDI system.
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef BCI_ESCAPE_H
#define BCI_ESCAPE_H

#include "c23_compat.h"
#include "../ast/bci_ast_extended.h"

// --- Escape Kind ---
typedef enum {
    ESCAPE_NONE,
    ESCAPE_HEAP,
    ESCAPE_RETURN,
    ESCAPE_GLOBAL,
    ESCAPE_PARAMETER
} EscapeKind;

// --- Escape Information ---
typedef struct {
    const char* var_name;
    EscapeKind kind;
    bool can_stack_allocate;
} EscapeInfo;

// --- Escape Analyzer ---
typedef struct {
    BciVec(EscapeInfo) escape_info;
} EscapeAnalyzer;

// --- Escape Analysis API ---

void escape_analyzer_init(EscapeAnalyzer* analyzer);
void escape_analyzer_free(EscapeAnalyzer* analyzer);

void escape_analyze_program(EscapeAnalyzer* analyzer, AstNode* program);
void escape_analyze_function(EscapeAnalyzer* analyzer, AstNode* func);

[[nodiscard]] EscapeInfo* escape_get_info(EscapeAnalyzer* analyzer, const char* var_name);
[[nodiscard]] bool escape_can_stack_allocate(EscapeAnalyzer* analyzer, const char* var_name);

// Compile-time invariants
static_assert(sizeof(void*) >= 4, "Escape analyzer requires at least 32-bit pointers");

#endif // BCI_ESCAPE_H
