
#ifndef BDI_AST_ML_H
#define BDI_AST_ML_H

#include <stdbool.h>
#include <stddef.h>

// AST node type (simplified)
typedef enum {
    AST_NODE_FUNCTION,
    AST_NODE_STATEMENT,
    AST_NODE_EXPRESSION,
    AST_NODE_DECLARATION
} ASTNodeType;

// AST node (simplified)
typedef struct ASTNode {
    ASTNodeType type;
    char name[64];
    struct ASTNode *children;
    size_t child_count;
    void *data;
} ASTNode;

// AST transformation
typedef struct {
    char description[256];
    double confidence;
    ASTNode *original;
    ASTNode *transformed;
} ASTTransformation;

// Initialize AST ML
bool ast_ml_init(void);

// Cleanup AST ML
void ast_ml_cleanup(void);

// Learn AST transformation
bool ast_ml_learn_transformation(const ASTNode *before, const ASTNode *after);

// Apply learned transformations
ASTNode* ast_ml_optimize_ast(const ASTNode *ast);

// Predict bugs in AST
bool ast_ml_predict_bug(const ASTNode *ast, char *bug_description, size_t desc_size);

// Detect dead code
bool ast_ml_detect_dead_code(const ASTNode *ast);

// Free AST node
void ast_ml_free_node(ASTNode *node);

#endif // BDI_AST_ML_H
