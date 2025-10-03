
// ===================================================================
// DESC: Implementation of Type Inference Engine
// ===================================================================

#include "c23_compat.h"
#include "bci_type_inference.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// --- Type Inference Context ---

void type_inference_init(TypeInferenceContext* ctx) {
    bci_vec_init(&ctx->type_vars);
    bci_vec_init(&ctx->constraints);
    ctx->next_var_id = 0;
}

void type_inference_free(TypeInferenceContext* ctx) {
    if (!ctx) return;
    
    for (size_t i = 0; i < ctx->type_vars.len; i++) {
        free((void*)ctx->type_vars.data[i].name);
        bci_vec_free(&ctx->type_vars.data[i].constraints);
    }
    bci_vec_free(&ctx->type_vars);
    bci_vec_free(&ctx->constraints);
}

// --- Type Variable Management ---

TypeVariable* type_inference_new_var(TypeInferenceContext* ctx, const char* name) {
    TypeVariable var;
    var.id = ctx->next_var_id++;
    var.name = name ? strdup(name) : nullptr;
    var.bound_type = nullptr;
    bci_vec_init(&var.constraints);
    
    bci_vec_push(&ctx->type_vars, var);
    return &ctx->type_vars.data[ctx->type_vars.len - 1];
}

void type_inference_add_constraint(TypeInferenceContext* ctx, TypeConstraint constraint) {
    bci_vec_push(&ctx->constraints, constraint);
}

// --- Constraint Solving ---

bool type_inference_solve(TypeInferenceContext* ctx) {
    // Simplified constraint solver
    bool changed = true;
    int iterations = 0;
    const int max_iterations = 100;
    
    while (changed && iterations < max_iterations) {
        changed = false;
        iterations++;
        
        for (size_t i = 0; i < ctx->constraints.len; i++) {
            TypeConstraint* c = &ctx->constraints.data[i];
            
            switch (c->kind) {
                case CONSTRAINT_EQUALITY:
                    if (unify_types(ctx, c->left, c->right)) {
                        changed = true;
                    }
                    break;
                
                case CONSTRAINT_SUBTYPE:
                    // Simplified subtype checking
                    if (bci_type_ext_is_assignable(c->left, c->right)) {
                        changed = true;
                    }
                    break;
                
                default:
                    break;
            }
        }
    }
    
    return iterations < max_iterations;
}

BciTypeExt* type_inference_get_type(TypeInferenceContext* ctx, TypeVariable* var) {
    if (!ctx || !var) return nullptr;
    return var->bound_type;
}

// --- Type Inference for Expressions ---

BciTypeExt* infer_expr_type(TypeInferenceContext* ctx, AstNode* expr) {
    if (!ctx || !expr) return nullptr;
    
    switch (expr->kind) {
        case AST_NODE_LITERAL:
            // Return literal type
            return nullptr; // Simplified
        
        case AST_NODE_BINARY_OP:
            // Infer from operands
            return nullptr; // Simplified
        
        case AST_NODE_LAMBDA:
            return infer_lambda_type(ctx, expr);
        
        default:
            return nullptr;
    }
}

BciTypeExt* infer_lambda_type(TypeInferenceContext* ctx, AstNode* lambda) {
    if (!ctx || !lambda || lambda->kind != AST_NODE_LAMBDA) return nullptr;
    
    // Create function type
    BciTypeExt* return_type = nullptr; // Infer from body
    BciTypeExt* func_type = bci_type_function_create(return_type, false);
    
    // Add parameter types
    for (size_t i = 0; i < lambda->as.lambda.params.len; i++) {
        AstVarDecl* param = lambda->as.lambda.params.data[i];
        BciTypeExt* param_type = (BciTypeExt*)param->type;
        bci_type_function_add_param(func_type, param_type);
    }
    
    return func_type;
}

// --- Unification ---

bool unify_types(TypeInferenceContext* ctx, BciTypeExt* a, BciTypeExt* b) {
    (void)ctx;
    
    if (!a || !b) return false;
    if (a == b) return true;
    
    // Simplified unification
    return bci_type_ext_equals(a, b);
}
