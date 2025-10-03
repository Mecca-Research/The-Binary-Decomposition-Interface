
// ===================================================================
// DESC: Implementation of Escape Analysis
// ===================================================================

#include "c23_compat.h"
#include "bci_escape.h"
#include <stdlib.h>
#include <string.h>

// --- Escape Analyzer ---

void escape_analyzer_init(EscapeAnalyzer* analyzer) {
    bci_vec_init(&analyzer->escape_info);
}

void escape_analyzer_free(EscapeAnalyzer* analyzer) {
    if (!analyzer) return;
    
    for (size_t i = 0; i < analyzer->escape_info.len; i++) {
        free((void*)analyzer->escape_info.data[i].var_name);
    }
    bci_vec_free(&analyzer->escape_info);
}

// --- Analysis Functions ---

void escape_analyze_program(EscapeAnalyzer* analyzer, AstNode* program) {
    if (!analyzer || !program) return;
    
    // Traverse program and analyze escapes
    // Simplified implementation
}

void escape_analyze_function(EscapeAnalyzer* analyzer, AstNode* func) {
    if (!analyzer || !func || func->kind != AST_NODE_FUNC_DECL) return;
    
    // Analyze function for escaping values
    // Check returns, heap allocations, global assignments
}

EscapeInfo* escape_get_info(EscapeAnalyzer* analyzer, const char* var_name) {
    if (!analyzer || !var_name) return nullptr;
    
    for (size_t i = 0; i < analyzer->escape_info.len; i++) {
        if (strcmp(analyzer->escape_info.data[i].var_name, var_name) == 0) {
            return &analyzer->escape_info.data[i];
        }
    }
    return nullptr;
}

bool escape_can_stack_allocate(EscapeAnalyzer* analyzer, const char* var_name) {
    EscapeInfo* info = escape_get_info(analyzer, var_name);
    if (!info) return true; // Default to stack
    
    return info->can_stack_allocate;
}
