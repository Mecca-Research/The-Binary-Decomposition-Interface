
#ifndef BDI_LEXER_ML_H
#define BDI_LEXER_ML_H

#include <stdbool.h>
#include <stddef.h>

// Syntax error types
typedef enum {
    SYNTAX_ERROR_MISSING_SEMICOLON,
    SYNTAX_ERROR_MISSING_BRACE,
    SYNTAX_ERROR_MISSING_PAREN,
    SYNTAX_ERROR_INVALID_TOKEN,
    SYNTAX_ERROR_UNKNOWN
} SyntaxErrorType;

// Syntax error
typedef struct {
    SyntaxErrorType type;
    size_t line;
    size_t column;
    char message[256];
    char suggested_fix[256];
    double confidence;
} SyntaxError;

// Auto-repair result
typedef struct {
    bool success;
    char *repaired_code;
    size_t repair_count;
    SyntaxError *errors;
    size_t error_count;
} AutoRepairResult;

// Initialize lexer ML
bool lexer_ml_init(void);

// Cleanup lexer ML
void lexer_ml_cleanup(void);

// Auto-repair syntax errors
AutoRepairResult* lexer_ml_auto_repair(const char *code);

// Free auto-repair result
void lexer_ml_free_repair_result(AutoRepairResult *result);

// Predict next token
const char* lexer_ml_predict_next_token(const char *code, size_t position);

// Suggest construct completion
const char* lexer_ml_suggest_completion(const char *code, size_t position);

// Detect common mistakes
bool lexer_ml_detect_mistake(const char *code, SyntaxError *error);

#endif // BDI_LEXER_ML_H
