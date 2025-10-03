// ===================================================================
// DESC: Implementation of the Semantic Analyzer. Traverses the AST
//       to perform semantic checks and build the symbol table.
// ===================================================================

#include "c23_compat.h"
#include "bci_analyzer.h"
#include <stdio.h>
#include <stdlib.h>

// --- Forward Declarations for Recursive Traversal ---
static void analyze_node(Analyzer* analyzer, AstNode* node);
static void analyze_block(Analyzer* analyzer, AstBlock* block);

// --- Helper for Reporting Errors ---
static void error(Analyzer* analyzer, int line, const char* message) {
    fprintf(stderr, "[line %d] Semantic Error: %s\n", line, message);
    analyzer->had_error = true;
}

// --- AST Traversal Functions ---

// Handles a binary operation node.
static void analyze_binary_op(Analyzer* analyzer, AstNode* node) {
    analyze_node(analyzer, node->as.binary_op.left);
    analyze_node(analyzer, node->as.binary_op.right);

    // Basic type check: for now, we only support operations on integers.
    BciType* left_type = node->as.binary_op.left->resolved_type;
    BciType* right_type = node->as.binary_op.right->resolved_type;

    if (left_type && right_type) {
        if (left_type->kind != BCI_TYPE_I64 || right_type->kind != BCI_TYPE_I64) {
            error(analyzer, node->line, "Operands must be integers.");
        }
    }
    
    // The result of an arithmetic operation is an integer.
    // A proper implementation would have a type promotion system.
    node->resolved_type = malloc(sizeof(BciType));
    node->resolved_type->kind = BCI_TYPE_I64;
    node->resolved_type->base = nullptr;
}

// Handles a literal node.
static void analyze_literal(Analyzer* analyzer, AstNode* node) {
    // The type is inherent to the literal. For now, we only have i64.
    node->resolved_type = malloc(sizeof(BciType));
    node->resolved_type->kind = BCI_TYPE_I64;
    node->resolved_type->base = nullptr;
}

// Main dispatcher for traversing any AST node.
static void analyze_node(Analyzer* analyzer, AstNode* node) {
    if (!node) return;

    switch (node->kind) {
        case AST_NODE_LITERAL:
            analyze_literal(analyzer, node);
            break;
        case AST_NODE_BINARY_OP:
            analyze_binary_op(analyzer, node);
            break;
        case AST_NODE_BLOCK:
        case AST_NODE_PROGRAM:
            analyze_block(analyzer, node->as.block);
            break;
        // Other cases for statements, declarations, etc. will go here.
        default:
            // This case should not be reached in a complete analyzer.
            break;
    }
}

// Traverses a block of statements within a new scope.
static void analyze_block(Analyzer* analyzer, AstBlock* block) {
    symbol_table_begin_scope(analyzer->symbol_table);
    for (size_t i = 0; i < block->len; ++i) {
        analyze_node(analyzer, block->data[i]);
    }
    symbol_table_end_scope(analyzer->symbol_table);
}


// --- Analyzer Public API Implementation ---

void analyzer_init(Analyzer* analyzer) {
    analyzer->symbol_table = malloc(sizeof(SymbolTable));
    symbol_table_init(analyzer->symbol_table);
    analyzer->had_error = false;
}

void analyzer_free(Analyzer* analyzer) {
    symbol_table_free(analyzer->symbol_table);
    free(analyzer->symbol_table);
}

bool analyzer_analyze(Analyzer* analyzer, AstNode* program) {
    analyze_node(analyzer, program);
    return !analyzer->had_error;
}
