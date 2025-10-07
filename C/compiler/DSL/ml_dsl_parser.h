
/**
 * @file ml_dsl_parser.h
 * @brief ML DSL Parser - Syntax Analysis for ML Model Declaration Language
 * @details Parses token stream into Abstract Syntax Tree
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * 
 * @author BDI Kernel Team
 * @date 2025
 */
#ifndef BDI_ML_DSL_PARSER_H
#define BDI_ML_DSL_PARSER_H

#include "ml_dsl_lexer.h"
#include "ml_dsl_ast.h"
#include "../../c23_compat.h"
#include <stddef.h>
#include <stdbool.h>

// Parser state
typedef struct {
    Token* tokens;
    size_t token_count;
    size_t current;
    
    // Error tracking
    bool has_error;
    char* error_message;
    size_t error_line;
    size_t error_column;
    
    // Panic mode for error recovery
    bool panic_mode;
} Parser;

// Parser lifecycle
Parser* parser_create(Token* tokens, size_t token_count);
void parser_destroy(Parser* parser);

// Parsing
ASTNode* parser_parse(Parser* parser);
ASTNode* parser_parse_model_declaration(Parser* parser);
ASTNode* parser_parse_train_statement(Parser* parser);
ASTNode* parser_parse_predict_statement(Parser* parser);

// Error handling
bool parser_has_error(const Parser* parser);
const char* parser_get_error(const Parser* parser);
void parser_report_error(Parser* parser, const char* message);
void parser_synchronize(Parser* parser);

#endif // BDI_ML_DSL_PARSER_H
