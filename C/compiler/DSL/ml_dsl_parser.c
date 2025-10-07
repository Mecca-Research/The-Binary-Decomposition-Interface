
/**
 * @file ml_dsl_parser.c
 * @brief ML DSL Parser Implementation
 */

#include "ml_dsl_parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Helper functions

static Token* current_token(const Parser* parser) {
    if (!parser || parser->current >= parser->token_count) return nullptr;
    return &parser->tokens[parser->current];
}

static Token* peek_token(const Parser* parser, size_t offset) {
    size_t index = parser->current + offset;
    if (!parser || index >= parser->token_count) return nullptr;
    return &parser->tokens[index];
}

static bool is_at_end(const Parser* parser) {
    Token* token = current_token(parser);
    return !token || token->type == TOKEN_EOF;
}

static Token* advance(Parser* parser) {
    if (!is_at_end(parser)) {
        parser->current++;
    }
    return &parser->tokens[parser->current - 1];
}

static bool check(const Parser* parser, TokenType type) {
    Token* token = current_token(parser);
    return token && token->type == type;
}

static bool match(Parser* parser, TokenType type) {
    if (check(parser, type)) {
        advance(parser);
        return true;
    }
    return false;
}

static Token* consume(Parser* parser, TokenType type, const char* message) {
    if (check(parser, type)) {
        return advance(parser);
    }
    
    parser_report_error(parser, message);
    return nullptr;
}

// Parser lifecycle

Parser* parser_create(Token* tokens, size_t token_count) {
    if (!tokens || token_count == 0) return nullptr;
    
    Parser* parser = malloc(sizeof(Parser));
    if (!parser) return nullptr;
    
    parser->tokens = tokens;
    parser->token_count = token_count;
    parser->current = 0;
    parser->has_error = false;
    parser->error_message = nullptr;
    parser->error_line = 0;
    parser->error_column = 0;
    parser->panic_mode = false;
    
    return parser;
}

void parser_destroy(Parser* parser) {
    if (!parser) return;
    
    free(parser->error_message);
    free(parser);
}

// Error handling

bool parser_has_error(const Parser* parser) {
    return parser ? parser->has_error : false;
}

const char* parser_get_error(const Parser* parser) {
    return parser ? parser->error_message : nullptr;
}

void parser_report_error(Parser* parser, const char* message) {
    if (!parser || parser->panic_mode) return;
    
    parser->has_error = true;
    parser->panic_mode = true;
    
    Token* token = current_token(parser);
    if (token) {
        parser->error_line = token->line;
        parser->error_column = token->column;
    }
    
    free(parser->error_message);
    parser->error_message = message ? strdup(message) : nullptr;
    
    if (token) {
        fprintf(stderr, "Parse error at line %zu, column %zu: %s\n",
                token->line, token->column, message);
    }
}

void parser_synchronize(Parser* parser) {
    if (!parser) return;
    
    parser->panic_mode = false;
    
    while (!is_at_end(parser)) {
        Token* token = current_token(parser);
        
        // Synchronize at statement boundaries
        if (token->type == TOKEN_SEMICOLON) {
            advance(parser);
            return;
        }
        
        // Synchronize at declaration keywords
        if (token->type == TOKEN_ML ||
            token->type == TOKEN_TRAIN ||
            token->type == TOKEN_PREDICT) {
            return;
        }
        
        advance(parser);
    }
}

// Parsing helper functions

static ASTParameter* parse_parameter(Parser* parser) {
    Token* name_token = consume(parser, TOKEN_IDENTIFIER, "Expected parameter name");
    if (!name_token) return nullptr;
    
    if (!consume(parser, TOKEN_COLON, "Expected ':' after parameter name")) {
        return nullptr;
    }
    
    Token* value_token = current_token(parser);
    if (!value_token) {
        parser_report_error(parser, "Expected parameter value");
        return nullptr;
    }
    
    ASTParameter* param = ast_parameter_create(name_token->lexeme);
    if (!param) return nullptr;
    
    // Parse value based on type
    if (value_token->type == TOKEN_NUMBER) {
        param->value_type = PARAM_NUMBER;
        param->value.number = value_token->value.number;
        advance(parser);
    } else if (value_token->type == TOKEN_STRING) {
        param->value_type = PARAM_STRING;
        param->value.string = strdup(value_token->value.string);
        advance(parser);
    } else if (value_token->type == TOKEN_IDENTIFIER) {
        param->value_type = PARAM_IDENTIFIER;
        param->value.string = strdup(value_token->lexeme);
        advance(parser);
    } else if (value_token->type == TOKEN_VECTOR ||
               value_token->type == TOKEN_SCALAR ||
               value_token->type == TOKEN_CLASS ||
               value_token->type == TOKEN_CLUSTER ||
               value_token->type == TOKEN_DISCRETE) {
        param->value_type = PARAM_TYPE;
        param->value.string = strdup(value_token->lexeme);
        advance(parser);
        
        // Check for array dimension [n]
        if (match(parser, TOKEN_LBRACKET)) {
            Token* dim = consume(parser, TOKEN_IDENTIFIER, "Expected dimension");
            if (dim) {
                // Store dimension info
                char* full_type = malloc(strlen(param->value.string) + strlen(dim->lexeme) + 4);
                sprintf(full_type, "%s[%s]", param->value.string, dim->lexeme);
                free(param->value.string);
                param->value.string = full_type;
            }
            consume(parser, TOKEN_RBRACKET, "Expected ']'");
        }
    } else {
        // Try to parse as keyword value
        param->value_type = PARAM_IDENTIFIER;
        param->value.string = strdup(value_token->lexeme);
        advance(parser);
    }
    
    return param;
}

static ASTParameterList* parse_parameter_list(Parser* parser) {
    ASTParameterList* list = ast_parameter_list_create();
    if (!list) return nullptr;
    
    while (!is_at_end(parser) && !check(parser, TOKEN_RBRACE)) {
        ASTParameter* param = parse_parameter(parser);
        if (!param) {
            ast_parameter_list_destroy(list);
            return nullptr;
        }
        
        ast_parameter_list_add(list, param);
        
        if (!match(parser, TOKEN_SEMICOLON)) {
            break;
        }
    }
    
    return list;
}

// Main parsing functions

ASTNode* parser_parse_model_declaration(Parser* parser) {
    if (!parser) return nullptr;
    
    // ml model <name> { ... }
    if (!consume(parser, TOKEN_ML, "Expected 'ml' keyword")) {
        return nullptr;
    }
    
    if (!consume(parser, TOKEN_MODEL, "Expected 'model' keyword")) {
        return nullptr;
    }
    
    Token* name_token = consume(parser, TOKEN_IDENTIFIER, "Expected model name");
    if (!name_token) return nullptr;
    
    if (!consume(parser, TOKEN_LBRACE, "Expected '{' after model name")) {
        return nullptr;
    }
    
    ASTParameterList* params = parse_parameter_list(parser);
    if (!params) return nullptr;
    
    if (!consume(parser, TOKEN_RBRACE, "Expected '}' after model parameters")) {
        ast_parameter_list_destroy(params);
        return nullptr;
    }
    
    ASTNode* node = ast_model_decl_create(name_token->lexeme, params);
    return node;
}

ASTNode* parser_parse_train_statement(Parser* parser) {
    if (!parser) return nullptr;
    
    // train <model_name> with dataset "<path>" { ... }
    if (!consume(parser, TOKEN_TRAIN, "Expected 'train' keyword")) {
        return nullptr;
    }
    
    Token* model_name = consume(parser, TOKEN_IDENTIFIER, "Expected model name");
    if (!model_name) return nullptr;
    
    if (!consume(parser, TOKEN_WITH, "Expected 'with' keyword")) {
        return nullptr;
    }
    
    if (!consume(parser, TOKEN_DATASET, "Expected 'dataset' keyword")) {
        return nullptr;
    }
    
    Token* dataset_path = consume(parser, TOKEN_STRING, "Expected dataset path");
    if (!dataset_path) return nullptr;
    
    ASTParameterList* params = nullptr;
    
    if (match(parser, TOKEN_LBRACE)) {
        params = parse_parameter_list(parser);
        if (!params) return nullptr;
        
        if (!consume(parser, TOKEN_RBRACE, "Expected '}' after training parameters")) {
            ast_parameter_list_destroy(params);
            return nullptr;
        }
    }
    
    if (!consume(parser, TOKEN_SEMICOLON, "Expected ';' after train statement")) {
        if (params) ast_parameter_list_destroy(params);
        return nullptr;
    }
    
    ASTNode* node = ast_train_stmt_create(model_name->lexeme, 
                                          dataset_path->value.string, 
                                          params);
    return node;
}

ASTNode* parser_parse_predict_statement(Parser* parser) {
    if (!parser) return nullptr;
    
    // predict <model_name> on input [values...];
    if (!consume(parser, TOKEN_PREDICT, "Expected 'predict' keyword")) {
        return nullptr;
    }
    
    Token* model_name = consume(parser, TOKEN_IDENTIFIER, "Expected model name");
    if (!model_name) return nullptr;
    
    if (!consume(parser, TOKEN_ON, "Expected 'on' keyword")) {
        return nullptr;
    }
    
    if (!consume(parser, TOKEN_INPUT, "Expected 'input' keyword")) {
        return nullptr;
    }
    
    if (!consume(parser, TOKEN_LBRACKET, "Expected '[' before input values")) {
        return nullptr;
    }
    
    // Parse input values
    size_t capacity = 16;
    size_t count = 0;
    double* values = malloc(capacity * sizeof(double));
    
    if (!values) {
        parser_report_error(parser, "Memory allocation failed");
        return nullptr;
    }
    
    while (!is_at_end(parser) && !check(parser, TOKEN_RBRACKET)) {
        Token* value_token = consume(parser, TOKEN_NUMBER, "Expected numeric value");
        if (!value_token) {
            free(values);
            return nullptr;
        }
        
        if (count >= capacity) {
            capacity *= 2;
            double* new_values = realloc(values, capacity * sizeof(double));
            if (!new_values) {
                free(values);
                parser_report_error(parser, "Memory allocation failed");
                return nullptr;
            }
            values = new_values;
        }
        
        values[count++] = value_token->value.number;
        
        if (!check(parser, TOKEN_RBRACKET)) {
            if (!consume(parser, TOKEN_COMMA, "Expected ',' between values")) {
                free(values);
                return nullptr;
            }
        }
    }
    
    if (!consume(parser, TOKEN_RBRACKET, "Expected ']' after input values")) {
        free(values);
        return nullptr;
    }
    
    if (!consume(parser, TOKEN_SEMICOLON, "Expected ';' after predict statement")) {
        free(values);
        return nullptr;
    }
    
    ASTNode* node = ast_predict_stmt_create(model_name->lexeme, values, count);
    return node;
}

ASTNode* parser_parse(Parser* parser) {
    if (!parser) return nullptr;
    
    // Create program node (root)
    ASTNode* program = ast_program_create();
    if (!program) return nullptr;
    
    while (!is_at_end(parser)) {
        ASTNode* stmt = nullptr;
        
        Token* token = current_token(parser);
        if (!token) break;
        
        if (token->type == TOKEN_ML) {
            stmt = parser_parse_model_declaration(parser);
        } else if (token->type == TOKEN_TRAIN) {
            stmt = parser_parse_train_statement(parser);
        } else if (token->type == TOKEN_PREDICT) {
            stmt = parser_parse_predict_statement(parser);
        } else {
            parser_report_error(parser, "Unexpected token");
            parser_synchronize(parser);
            continue;
        }
        
        if (stmt) {
            ast_program_add_statement(program, stmt);
        } else if (parser->has_error) {
            parser_synchronize(parser);
        }
    }
    
    return program;
}
