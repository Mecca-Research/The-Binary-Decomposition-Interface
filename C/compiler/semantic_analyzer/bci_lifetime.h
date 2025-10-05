
// ===================================================================
// DESC: Lifetime Analysis for Phase 3.3
//       Tracks variable lifetimes and detects use-after-free
// ===================================================================
/**
 * @file bci_lifetime.h
 * @brief Bci Lifetime API
 * @details This file provides the bci lifetime functionality for the BDI system.
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef BCI_LIFETIME_H
#define BCI_LIFETIME_H

#include "c23_compat.h"
#include "../ast/bci_ast_extended.h"

// --- Lifetime Information ---
typedef struct {
    const char* var_name;
    int birth_line;
    int death_line;
    bool escapes;
    bool is_borrowed;
} Lifetime;

// --- Lifetime Analyzer ---
typedef struct {
    BciVec(Lifetime) lifetimes;
    int current_line;
} LifetimeAnalyzer;

// --- Lifetime Analysis API ---

void lifetime_analyzer_init(LifetimeAnalyzer* analyzer);
void lifetime_analyzer_free(LifetimeAnalyzer* analyzer);

void lifetime_analyze_program(LifetimeAnalyzer* analyzer, AstNode* program);
void lifetime_analyze_function(LifetimeAnalyzer* analyzer, AstNode* func);
void lifetime_analyze_block(LifetimeAnalyzer* analyzer, AstNode* block);

[[nodiscard]] Lifetime* lifetime_get(LifetimeAnalyzer* analyzer, const char* var_name);
[[nodiscard]] bool lifetime_is_live(LifetimeAnalyzer* analyzer, const char* var_name, 
                                     int line);
[[nodiscard]] bool lifetime_check_use_after_free(LifetimeAnalyzer* analyzer);

// Compile-time invariants
static_assert(sizeof(void*) >= 4, "Lifetime analyzer requires at least 32-bit pointers");

#endif // BCI_LIFETIME_H
