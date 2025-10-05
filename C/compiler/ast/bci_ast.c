// ===================================================================
// DESC: Implementation of AST node management functions.
// ===================================================================

#include "c23_compat.h"
#include "bci_ast.h"
#include <stdlib.h>
#include <stdio.h>

// --- AST Management Functions ---

AstNode* ast_new_node(AstNodeKind kind) {
    AstNode* node = calloc(1, sizeof(AstNode)); // calloc initializes to zero
    if (!node) {
        perror("Failed to allocate AstNode");
        return nullptr;
    }
    node->kind = kind;
    return node;
}

// Recursively frees an AST node and all its children.
void ast_free_node(AstNode* node) {
    if (!node) return;

    switch (node->kind) {
        case AST_NODE_BINARY_OP:
            ast_free_node(node->as.binary_op.left);
            ast_free_node(node->as.binary_op.right);
            break;
        case AST_NODE_BLOCK:
        case AST_NODE_PROGRAM:
            if (node->as.block) {
                for (size_t i = 0; i < node->as.block->len; ++i) {
                    ast_free_node(node->as.block->data[i]);
                }
                bci_vec_free(node->as.block);
                free(node->as.block);
            }
            break;
        case AST_NODE_VAR_DECL:
            ast_free_node(node->as.var_decl.initializer);
            break;
        case AST_NODE_FUNC_DECL:
            if (node->as.func_decl.params.data) {
                // Params are AstVarDecl, need careful freeing if they are complex
                for (size_t i = 0; i < node->as.func_decl.params.len; ++i) {
                    // Assuming params are simple and don't need deep free here
                    free(node->as.func_decl.params.data[i]);
                }
                bci_vec_free(&node->as.func_decl.params);
            }
            ast_free_node((AstNode*)node->as.func_decl.body); // Cast AstBlock back
            break;
        // Add cases for other node kinds that have heap-allocated children
        case AST_NODE_LITERAL: // No children to free
        case AST_NODE_VARIABLE: // No children to free
        default:
            break;
    }

    free(node);
}

// --- AST Node Factory Functions ---

AstNode* ast_new_literal_int(int64_t value) {
    AstNode* node = ast_new_node(AST_NODE_LITERAL);
    if (!node) return nullptr;
    // For now, treat integers as doubles for simplicity (VM uses double stack)
    node->as.literal.type = nullptr;
    node->as.literal.value.f64 = (double)value;
    return node;
}

AstNode* ast_new_literal_float(double value) {
    AstNode* node = ast_new_node(AST_NODE_LITERAL);
    if (!node) return nullptr;
    node->as.literal.type = nullptr; // Type will be set during semantic analysis
    node->as.literal.value.f64 = value;
    return node;
}

AstNode* ast_new_binary_op(const char* op, AstNode* left, AstNode* right) {
    AstNode* node = ast_new_node(AST_NODE_BINARY_OP);
    if (!node) return nullptr;
    node->as.binary_op.op = op;
    node->as.binary_op.left = left;
    node->as.binary_op.right = right;
    return node;
}

AstNode* ast_new_program() {
    AstNode* node = ast_new_node(AST_NODE_PROGRAM);
    if (!node) return nullptr;
    node->as.block = malloc(sizeof(AstBlock));
    if (!node->as.block) {
        perror("Failed to allocate AstBlock for program");
        free(node);
        return nullptr;
    }
    bci_vec_init(node->as.block);
    return node;
}
