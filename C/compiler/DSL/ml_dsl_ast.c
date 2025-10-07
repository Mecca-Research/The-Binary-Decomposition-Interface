
/**
 * @file ml_dsl_ast.c
 * @brief ML DSL Abstract Syntax Tree Implementation
 */

#include "ml_dsl_ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Parameter functions

ASTParameter* ast_parameter_create(const char* name) {
    if (!name) return nullptr;
    
    ASTParameter* param = malloc(sizeof(ASTParameter));
    if (!param) return nullptr;
    
    param->name = strdup(name);
    param->value_type = PARAM_NUMBER;
    param->value.number = 0.0;
    
    return param;
}

void ast_parameter_destroy(ASTParameter* param) {
    if (!param) return;
    
    free(param->name);
    if (param->value_type == PARAM_STRING || 
        param->value_type == PARAM_IDENTIFIER ||
        param->value_type == PARAM_TYPE) {
        free(param->value.string);
    }
    free(param);
}

ASTParameter* ast_parameter_copy(const ASTParameter* param) {
    if (!param) return nullptr;
    
    ASTParameter* copy = malloc(sizeof(ASTParameter));
    if (!copy) return nullptr;
    
    copy->name = strdup(param->name);
    copy->value_type = param->value_type;
    
    if (param->value_type == PARAM_NUMBER) {
        copy->value.number = param->value.number;
    } else {
        copy->value.string = param->value.string ? strdup(param->value.string) : nullptr;
    }
    
    return copy;
}

// Parameter list functions

ASTParameterList* ast_parameter_list_create(void) {
    ASTParameterList* list = malloc(sizeof(ASTParameterList));
    if (!list) return nullptr;
    
    list->capacity = 16;
    list->count = 0;
    list->parameters = malloc(list->capacity * sizeof(ASTParameter*));
    
    if (!list->parameters) {
        free(list);
        return nullptr;
    }
    
    return list;
}

void ast_parameter_list_destroy(ASTParameterList* list) {
    if (!list) return;
    
    for (size_t i = 0; i < list->count; i++) {
        ast_parameter_destroy(list->parameters[i]);
    }
    
    free(list->parameters);
    free(list);
}

void ast_parameter_list_add(ASTParameterList* list, ASTParameter* param) {
    if (!list || !param) return;
    
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        ASTParameter** new_params = realloc(list->parameters,
                                           list->capacity * sizeof(ASTParameter*));
        if (!new_params) return;
        list->parameters = new_params;
    }
    
    list->parameters[list->count++] = param;
}

ASTParameter* ast_parameter_list_get(const ASTParameterList* list, const char* name) {
    if (!list || !name) return nullptr;
    
    for (size_t i = 0; i < list->count; i++) {
        if (strcmp(list->parameters[i]->name, name) == 0) {
            return list->parameters[i];
        }
    }
    
    return nullptr;
}

// AST node creation

ASTNode* ast_program_create(void) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) return nullptr;
    
    node->type = AST_PROGRAM;
    node->data.program.statement_capacity = 16;
    node->data.program.statement_count = 0;
    node->data.program.statements = malloc(node->data.program.statement_capacity * 
                                          sizeof(ASTNode*));
    
    if (!node->data.program.statements) {
        free(node);
        return nullptr;
    }
    
    return node;
}

ASTNode* ast_model_decl_create(const char* name, ASTParameterList* params) {
    if (!name || !params) return nullptr;
    
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) return nullptr;
    
    node->type = AST_MODEL_DECL;
    node->data.model_decl.name = strdup(name);
    node->data.model_decl.parameters = params;
    
    return node;
}

ASTNode* ast_train_stmt_create(const char* model_name, const char* dataset_path,
                               ASTParameterList* params) {
    if (!model_name || !dataset_path) return nullptr;
    
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) return nullptr;
    
    node->type = AST_TRAIN_STMT;
    node->data.train_stmt.model_name = strdup(model_name);
    node->data.train_stmt.dataset_path = strdup(dataset_path);
    node->data.train_stmt.parameters = params;
    
    return node;
}

ASTNode* ast_predict_stmt_create(const char* model_name, double* input_values,
                                 size_t input_count) {
    if (!model_name || !input_values) return nullptr;
    
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) return nullptr;
    
    node->type = AST_PREDICT_STMT;
    node->data.predict_stmt.model_name = strdup(model_name);
    node->data.predict_stmt.input_count = input_count;
    node->data.predict_stmt.input_values = malloc(input_count * sizeof(double));
    
    if (!node->data.predict_stmt.input_values) {
        free(node->data.predict_stmt.model_name);
        free(node);
        return nullptr;
    }
    
    memcpy(node->data.predict_stmt.input_values, input_values, 
           input_count * sizeof(double));
    
    return node;
}

// AST node destruction

void ast_node_destroy(ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_PROGRAM:
            for (size_t i = 0; i < node->data.program.statement_count; i++) {
                ast_node_destroy(node->data.program.statements[i]);
            }
            free(node->data.program.statements);
            break;
            
        case AST_MODEL_DECL:
            free(node->data.model_decl.name);
            ast_parameter_list_destroy(node->data.model_decl.parameters);
            break;
            
        case AST_TRAIN_STMT:
            free(node->data.train_stmt.model_name);
            free(node->data.train_stmt.dataset_path);
            if (node->data.train_stmt.parameters) {
                ast_parameter_list_destroy(node->data.train_stmt.parameters);
            }
            break;
            
        case AST_PREDICT_STMT:
            free(node->data.predict_stmt.model_name);
            free(node->data.predict_stmt.input_values);
            break;
            
        default:
            break;
    }
    
    free(node);
}

// AST manipulation

void ast_program_add_statement(ASTNode* program, ASTNode* statement) {
    if (!program || program->type != AST_PROGRAM || !statement) return;
    
    if (program->data.program.statement_count >= program->data.program.statement_capacity) {
        program->data.program.statement_capacity *= 2;
        ASTNode** new_stmts = realloc(program->data.program.statements,
                                     program->data.program.statement_capacity * sizeof(ASTNode*));
        if (!new_stmts) return;
        program->data.program.statements = new_stmts;
    }
    
    program->data.program.statements[program->data.program.statement_count++] = statement;
}

// AST printing

static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

void ast_print_parameter(const ASTParameter* param, int indent) {
    if (!param) return;
    
    print_indent(indent);
    printf("%s: ", param->name);
    
    switch (param->value_type) {
        case PARAM_NUMBER:
            printf("%.2f", param->value.number);
            break;
        case PARAM_STRING:
            printf("\"%s\"", param->value.string);
            break;
        case PARAM_IDENTIFIER:
        case PARAM_TYPE:
            printf("%s", param->value.string);
            break;
    }
    
    printf("\n");
}

void ast_print(const ASTNode* node, int indent) {
    if (!node) return;
    
    print_indent(indent);
    
    switch (node->type) {
        case AST_PROGRAM:
            printf("Program:\n");
            for (size_t i = 0; i < node->data.program.statement_count; i++) {
                ast_print(node->data.program.statements[i], indent + 1);
            }
            break;
            
        case AST_MODEL_DECL:
            printf("ModelDecl: %s\n", node->data.model_decl.name);
            if (node->data.model_decl.parameters) {
                for (size_t i = 0; i < node->data.model_decl.parameters->count; i++) {
                    ast_print_parameter(node->data.model_decl.parameters->parameters[i],
                                      indent + 1);
                }
            }
            break;
            
        case AST_TRAIN_STMT:
            printf("TrainStmt: %s\n", node->data.train_stmt.model_name);
            print_indent(indent + 1);
            printf("dataset: \"%s\"\n", node->data.train_stmt.dataset_path);
            if (node->data.train_stmt.parameters) {
                for (size_t i = 0; i < node->data.train_stmt.parameters->count; i++) {
                    ast_print_parameter(node->data.train_stmt.parameters->parameters[i],
                                      indent + 1);
                }
            }
            break;
            
        case AST_PREDICT_STMT:
            printf("PredictStmt: %s\n", node->data.predict_stmt.model_name);
            print_indent(indent + 1);
            printf("input: [");
            for (size_t i = 0; i < node->data.predict_stmt.input_count; i++) {
                printf("%.2f", node->data.predict_stmt.input_values[i]);
                if (i < node->data.predict_stmt.input_count - 1) {
                    printf(", ");
                }
            }
            printf("]\n");
            break;
            
        default:
            printf("Unknown node type\n");
            break;
    }
}
