
// ===================================================================
// DESC: Implementation of Extended AST for Phase 3
// ===================================================================

#include "c23_compat.h"
#include "bci_ast_extended.h"
#include <stdlib.h>
#include <string.h>

// --- Match Expression ---

AstNode* ast_new_match_expr(AstNode* scrutinee) {
    AstNode* node = ast_new_node(AST_NODE_MATCH_EXPR);
    if (!node) return nullptr;
    
    node->as.match_expr.scrutinee = scrutinee;
    bci_vec_init(&node->as.match_expr.arms);
    
    return node;
}

void ast_match_add_arm(AstNode* match_expr, AstPattern* pattern, AstNode* body) {
    if (!match_expr || match_expr->kind != AST_NODE_MATCH_EXPR) return;
    
    AstMatchArm arm;
    arm.pattern = pattern;
    arm.guard = nullptr;
    arm.body = body;
    
    bci_vec_push(&match_expr->as.match_expr.arms, arm);
}

// --- Pattern Creation ---

AstPattern* ast_new_pattern_wildcard(void) {
    AstPattern* pattern = malloc(sizeof(AstPattern));
    if (!pattern) return nullptr;
    
    pattern->kind = PATTERN_WILDCARD;
    return pattern;
}

AstPattern* ast_new_pattern_literal(AstNode* value) {
    AstPattern* pattern = malloc(sizeof(AstPattern));
    if (!pattern) return nullptr;
    
    pattern->kind = PATTERN_LITERAL;
    pattern->as.literal.value = value;
    return pattern;
}

AstPattern* ast_new_pattern_binding(const char* name) {
    AstPattern* pattern = malloc(sizeof(AstPattern));
    if (!pattern) return nullptr;
    
    pattern->kind = PATTERN_BINDING;
    pattern->as.binding.name = name ? strdup(name) : nullptr;
    return pattern;
}

AstPattern* ast_new_pattern_struct(const char* struct_name) {
    AstPattern* pattern = malloc(sizeof(AstPattern));
    if (!pattern) return nullptr;
    
    pattern->kind = PATTERN_STRUCT;
    pattern->as.struct_pattern.struct_name = struct_name ? strdup(struct_name) : nullptr;
    bci_vec_init(&pattern->as.struct_pattern.fields);
    return pattern;
}

void ast_pattern_free(AstPattern* pattern) {
    if (!pattern) return;
    
    switch (pattern->kind) {
        case PATTERN_BINDING:
            free((void*)pattern->as.binding.name);
            break;
        case PATTERN_STRUCT:
            free((void*)pattern->as.struct_pattern.struct_name);
            for (size_t i = 0; i < pattern->as.struct_pattern.fields.len; i++) {
                ast_pattern_free(pattern->as.struct_pattern.fields.data[i]);
            }
            bci_vec_free(&pattern->as.struct_pattern.fields);
            break;
        case PATTERN_LITERAL:
            if (pattern->as.literal.value) {
                ast_free_node(pattern->as.literal.value);
            }
            break;
        default:
            break;
    }
    
    free(pattern);
}

// --- Lambda Expression ---

AstNode* ast_new_lambda(void) {
    AstNode* node = ast_new_node(AST_NODE_LAMBDA);
    if (!node) return nullptr;
    
    bci_vec_init(&node->as.lambda.captures);
    bci_vec_init(&node->as.lambda.params);
    node->as.lambda.body = nullptr;
    node->as.lambda.inferred_type = nullptr;
    
    return node;
}

void ast_lambda_add_param(AstNode* lambda, const char* name, BciTypeExt* type) {
    if (!lambda || lambda->kind != AST_NODE_LAMBDA) return;
    
    AstVarDecl* param = malloc(sizeof(AstVarDecl));
    if (!param) return;
    
    param->name = name ? strdup(name) : nullptr;
    param->type = (BciType*)type; // Cast for compatibility
    param->initializer = nullptr;
    
    bci_vec_push(&lambda->as.lambda.params, param);
}

void ast_lambda_add_capture(AstNode* lambda, const char* name) {
    if (!lambda || lambda->kind != AST_NODE_LAMBDA) return;
    
    const char* capture = name ? strdup(name) : nullptr;
    bci_vec_push(&lambda->as.lambda.captures, capture);
}

void ast_lambda_set_body(AstNode* lambda, AstNode* body) {
    if (!lambda || lambda->kind != AST_NODE_LAMBDA) return;
    lambda->as.lambda.body = body;
}
