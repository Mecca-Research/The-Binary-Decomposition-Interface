
// ===================================================================
// DESC: Implementation of Lifetime Analysis
// ===================================================================

#include "c23_compat.h"
#include "bci_lifetime.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// --- Lifetime Analyzer ---

void lifetime_analyzer_init(LifetimeAnalyzer* analyzer) {
    bci_vec_init(&analyzer->lifetimes);
    analyzer->current_line = 0;
}

void lifetime_analyzer_free(LifetimeAnalyzer* analyzer) {
    if (!analyzer) return;
    
    for (size_t i = 0; i < analyzer->lifetimes.len; i++) {
        free((void*)analyzer->lifetimes.data[i].var_name);
    }
    bci_vec_free(&analyzer->lifetimes);
}

// --- Analysis Functions ---

void lifetime_analyze_program(LifetimeAnalyzer* analyzer, AstNode* program) {
    if (!analyzer || !program) return;
    
    // Traverse program AST
    if (program->kind == AST_NODE_PROGRAM && program->as.block) {
        lifetime_analyze_block(analyzer, (AstNode*)program->as.block);
    }
}

void lifetime_analyze_function(LifetimeAnalyzer* analyzer, AstNode* func) {
    if (!analyzer || !func || func->kind != AST_NODE_FUNC_DECL) return;
    
    // Analyze function body
    if (func->as.func_decl.body) {
        lifetime_analyze_block(analyzer, (AstNode*)func->as.func_decl.body);
    }
}

void lifetime_analyze_block(LifetimeAnalyzer* analyzer, AstNode* block) {
    if (!analyzer || !block) return;
    
    analyzer->current_line++;
    
    // Simplified: would traverse all statements in block
    // and track variable declarations and last uses
}

Lifetime* lifetime_get(LifetimeAnalyzer* analyzer, const char* var_name) {
    if (!analyzer || !var_name) return nullptr;
    
    for (size_t i = 0; i < analyzer->lifetimes.len; i++) {
        if (strcmp(analyzer->lifetimes.data[i].var_name, var_name) == 0) {
            return &analyzer->lifetimes.data[i];
        }
    }
    return nullptr;
}

bool lifetime_is_live(LifetimeAnalyzer* analyzer, const char* var_name, int line) {
    Lifetime* lt = lifetime_get(analyzer, var_name);
    if (!lt) return false;
    
    return line >= lt->birth_line && line <= lt->death_line;
}

bool lifetime_check_use_after_free(LifetimeAnalyzer* analyzer) {
    if (!analyzer) return false;
    
    // Simplified: would check for uses after death_line
    return true;
}
