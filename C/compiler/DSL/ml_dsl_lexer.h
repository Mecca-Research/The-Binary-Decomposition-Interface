
/**
 * @file ml_dsl_lexer.h
 * @brief ML DSL Lexer - Tokenization for ML Model Declaration Language
 * @details Lexical analysis for the BDI ML DSL
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * 
 * @author BDI Kernel Team
 * @date 2025
 */
#ifndef BDI_ML_DSL_LEXER_H
#define BDI_ML_DSL_LEXER_H

#include "../../c23_compat.h"
#include <stddef.h>
#include <stdbool.h>

// Token types
typedef enum {
    // Keywords
    TOKEN_ML,
    TOKEN_MODEL,
    TOKEN_TYPE,
    TOKEN_INPUT,
    TOKEN_OUTPUT,
    TOKEN_TRAIN,
    TOKEN_PREDICT,
    TOKEN_WITH,
    TOKEN_ON,
    TOKEN_DATASET,
    TOKEN_EPOCHS,
    TOKEN_BATCH_SIZE,
    
    // ML algorithm types
    TOKEN_LINEAR_REGRESSION,
    TOKEN_DECISION_TREE,
    TOKEN_SVM,
    TOKEN_KMEANS,
    TOKEN_QLEARNING,
    
    // Parameter names
    TOKEN_LEARNING_RATE,
    TOKEN_MAX_DEPTH,
    TOKEN_MIN_SAMPLES_SPLIT,
    TOKEN_CRITERION,
    TOKEN_KERNEL,
    TOKEN_C_PARAM,
    TOKEN_GAMMA,
    TOKEN_N_CLUSTERS,
    TOKEN_MAX_ITERATIONS,
    TOKEN_STATE_SPACE,
    TOKEN_ACTION_SPACE,
    TOKEN_DISCOUNT_FACTOR,
    TOKEN_EPSILON,
    
    // Data types
    TOKEN_VECTOR,
    TOKEN_SCALAR,
    TOKEN_CLASS,
    TOKEN_CLUSTER,
    TOKEN_DISCRETE,
    
    // Loss functions
    TOKEN_MSE,
    TOKEN_GINI,
    TOKEN_ENTROPY,
    
    // Optimizers
    TOKEN_GRADIENT_DESCENT,
    TOKEN_SGD,
    TOKEN_ADAM,
    
    // Kernels
    TOKEN_LINEAR,
    TOKEN_RBF,
    TOKEN_POLY,
    
    // Literals
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    
    // Operators and punctuation
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_DOT,
    TOKEN_ASSIGN,
    
    // Special
    TOKEN_EOF,
    TOKEN_ERROR,
    TOKEN_UNKNOWN
} TokenType;

// Token structure
typedef struct {
    TokenType type;
    char* lexeme;
    size_t length;
    size_t line;
    size_t column;
    
    // Value for literals
    union {
        double number;
        char* string;
    } value;
} Token;

// Lexer state
typedef struct {
    const char* source;
    size_t source_length;
    size_t current;
    size_t start;
    size_t line;
    size_t column;
    
    // Error tracking
    bool has_error;
    char* error_message;
    size_t error_line;
    size_t error_column;
} Lexer;

// Lexer lifecycle
Lexer* lexer_create(const char* source);
void lexer_destroy(Lexer* lexer);

// Tokenization
Token lexer_next_token(Lexer* lexer);
Token* lexer_tokenize_all(Lexer* lexer, size_t* out_count);

// Token utilities
const char* token_type_to_string(TokenType type);
void token_destroy(Token* token);
Token token_copy(const Token* token);
void token_print(const Token* token);

// Error handling
bool lexer_has_error(const Lexer* lexer);
const char* lexer_get_error(const Lexer* lexer);
void lexer_report_error(Lexer* lexer, const char* message);

// Compile-time invariants
static_assert(sizeof(TokenType) <= 4, "TokenType should fit in 32 bits");

#endif // BDI_ML_DSL_LEXER_H
