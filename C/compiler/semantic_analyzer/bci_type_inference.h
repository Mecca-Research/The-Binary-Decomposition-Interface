
// ===================================================================
// DESC: Type Inference Engine for Phase 3.3
//       Implements Hindley-Milner style type inference with constraints
// ===================================================================
#ifndef BCI_TYPE_INFERENCE_H
#define BCI_TYPE_INFERENCE_H

#include "c23_compat.h"
#include "../types/bci_types_extended.h"
#include "../ast/bci_ast_extended.h"

// --- Type Variable ---
typedef struct {
    int id;
    const char* name;
    BciTypeExt* bound_type;
    BciVec(BciTypeExt*) constraints;
} TypeVariable;

// --- Type Constraint ---
typedef enum {
    CONSTRAINT_EQUALITY,
    CONSTRAINT_SUBTYPE,
    CONSTRAINT_HAS_FIELD,
    CONSTRAINT_CALLABLE
} ConstraintKind;

typedef struct {
    ConstraintKind kind;
    BciTypeExt* left;
    BciTypeExt* right;
    const char* field_name; // For HAS_FIELD
} TypeConstraint;

// --- Type Inference Context ---
typedef struct {
    BciVec(TypeVariable) type_vars;
    BciVec(TypeConstraint) constraints;
    int next_var_id;
} TypeInferenceContext;

// --- Type Inference API ---

void type_inference_init(TypeInferenceContext* ctx);
void type_inference_free(TypeInferenceContext* ctx);

[[nodiscard]] TypeVariable* type_inference_new_var(TypeInferenceContext* ctx, 
                                                     const char* name);
void type_inference_add_constraint(TypeInferenceContext* ctx, TypeConstraint constraint);

[[nodiscard]] bool type_inference_solve(TypeInferenceContext* ctx);
[[nodiscard]] BciTypeExt* type_inference_get_type(TypeInferenceContext* ctx, 
                                                    TypeVariable* var);

// Infer types for AST nodes
[[nodiscard]] BciTypeExt* infer_expr_type(TypeInferenceContext* ctx, AstNode* expr);
[[nodiscard]] BciTypeExt* infer_lambda_type(TypeInferenceContext* ctx, AstNode* lambda);

// Unification
[[nodiscard]] bool unify_types(TypeInferenceContext* ctx, BciTypeExt* a, BciTypeExt* b);

// Compile-time invariants
static_assert(sizeof(void*) >= 4, "Type inference requires at least 32-bit pointers");

#endif // BCI_TYPE_INFERENCE_H
