
/**
 * @file ml_dsl_ast.h
 * @brief ML DSL Abstract Syntax Tree
 * @details AST representation for ML model declarations
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * 
 * @author BDI Kernel Team
 * @date 2025
 */
#ifndef BDI_ML_DSL_AST_H
#define BDI_ML_DSL_AST_H

#include "../../c23_compat.h"
#include <stddef.h>
#include <stdbool.h>

// Forward declarations
typedef struct ASTNode ASTNode;
typedef struct ASTParameter ASTParameter;
typedef struct ASTParameterList ASTParameterList;

// AST node types
typedef enum {
    AST_PROGRAM,
    AST_MODEL_DECL,
    AST_TRAIN_STMT,
    AST_PREDICT_STMT,
    AST_PARAMETER,
    AST_PARAMETER_LIST
} ASTNodeType;

// Parameter value types
typedef enum {
    PARAM_NUMBER,
    PARAM_STRING,
    PARAM_IDENTIFIER,
    PARAM_TYPE
} ParameterValueType;

// Parameter structure
struct ASTParameter {
    char* name;
    ParameterValueType value_type;
    union {
        double number;
        char* string;
    } value;
};

// Parameter list
struct ASTParameterList {
    ASTParameter** parameters;
    size_t count;
    size_t capacity;
};

// AST node structure
struct ASTNode {
    ASTNodeType type;
    
    union {
        // Program (root)
        struct {
            ASTNode** statements;
            size_t statement_count;
            size_t statement_capacity;
        } program;
        
        // Model declaration
        struct {
            char* name;
            ASTParameterList* parameters;
        } model_decl;
        
        // Train statement
        struct {
            char* model_name;
            char* dataset_path;
            ASTParameterList* parameters;
        } train_stmt;
        
        // Predict statement
        struct {
            char* model_name;
            double* input_values;
            size_t input_count;
        } predict_stmt;
    } data;
};

// Parameter functions
ASTParameter* ast_parameter_create(const char* name);
void ast_parameter_destroy(ASTParameter* param);
ASTParameter* ast_parameter_copy(const ASTParameter* param);

// Parameter list functions
ASTParameterList* ast_parameter_list_create(void);
void ast_parameter_list_destroy(ASTParameterList* list);
void ast_parameter_list_add(ASTParameterList* list, ASTParameter* param);
ASTParameter* ast_parameter_list_get(const ASTParameterList* list, const char* name);

// AST node creation
ASTNode* ast_program_create(void);
ASTNode* ast_model_decl_create(const char* name, ASTParameterList* params);
ASTNode* ast_train_stmt_create(const char* model_name, const char* dataset_path,
                               ASTParameterList* params);
ASTNode* ast_predict_stmt_create(const char* model_name, double* input_values,
                                 size_t input_count);

// AST node destruction
void ast_node_destroy(ASTNode* node);

// AST manipulation
void ast_program_add_statement(ASTNode* program, ASTNode* statement);

// AST traversal and printing
void ast_print(const ASTNode* node, int indent);
void ast_print_parameter(const ASTParameter* param, int indent);

// Compile-time invariants
static_assert(sizeof(ASTNodeType) <= 4, "ASTNodeType should fit in 32 bits");

#endif // BDI_ML_DSL_AST_H
