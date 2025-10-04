
// ===================================================================
// DESC: Extended AST for Phase 3 - Pattern Matching and Lambdas
// ===================================================================
#ifndef BCI_AST_EXTENDED_H
#define BCI_AST_EXTENDED_H

#include "c23_compat.h"
#include "../ast/bci_ast.h"
#include "../types/bci_types_extended.h"

// --- Extended AST Node Types ---
typedef enum {
    AST_NODE_MATCH_EXPR = 100,
    AST_NODE_MATCH_ARM,
    AST_NODE_PATTERN,
    AST_NODE_PATTERN_WILDCARD,
    AST_NODE_PATTERN_LITERAL,
    AST_NODE_PATTERN_BINDING,
    AST_NODE_PATTERN_STRUCT,
    AST_NODE_LAMBDA,
    AST_NODE_CLOSURE,
    AST_NODE_TYPE_ANNOTATION
} AstNodeExtKind;

// --- Pattern Structures ---

typedef struct AstPattern AstPattern;

typedef enum {
    PATTERN_WILDCARD,
    PATTERN_LITERAL,
    PATTERN_BINDING,
    PATTERN_STRUCT,
    PATTERN_ENUM
} PatternKind;

struct AstPattern {
    PatternKind kind;
    union {
        struct {
            const char* name;
        } binding;
        struct {
            AstNode* value;
        } literal;
        struct {
            const char* struct_name;
            BciVec(AstPattern*) fields;
        } struct_pattern;
    } as;
};

// --- Forward Declarations ---
typedef struct AstMatchArm AstMatchArm;

// --- Match Expression ---
typedef struct {
    AstNode* scrutinee;
    BciVec(AstMatchArm) arms;
} AstMatchExpr;

struct AstMatchArm {
    AstPattern* pattern;
    AstNode* guard;
    AstNode* body;
};

// --- Lambda Expression ---
typedef struct {
    BciVec(const char*) captures;
    BciVec(AstVarDecl*) params;
    AstNode* body;
    BciTypeExt* inferred_type;
} AstLambda;

// --- Closure ---
typedef struct {
    AstLambda* lambda;
    BciVec(AstNode*) captured_values;
} AstClosure;

// --- Extended AST Functions ---

[[nodiscard]] AstNode* ast_new_match_expr(AstNode* scrutinee);
void ast_match_add_arm(AstNode* match_expr, AstPattern* pattern, AstNode* body);

[[nodiscard]] AstPattern* ast_new_pattern_wildcard(void);
[[nodiscard]] AstPattern* ast_new_pattern_literal(AstNode* value);
[[nodiscard]] AstPattern* ast_new_pattern_binding(const char* name);
[[nodiscard]] AstPattern* ast_new_pattern_struct(const char* struct_name);

[[nodiscard]] AstNode* ast_new_lambda(void);
void ast_lambda_add_param(AstNode* lambda, const char* name, BciTypeExt* type);
void ast_lambda_add_capture(AstNode* lambda, const char* name);
void ast_lambda_set_body(AstNode* lambda, AstNode* body);

void ast_pattern_free(AstPattern* pattern);

// Compile-time invariants
static_assert(sizeof(void*) >= 4, "Extended AST requires at least 32-bit pointers");

#endif // BCI_AST_EXTENDED_H
