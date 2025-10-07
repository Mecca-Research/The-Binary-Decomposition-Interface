
/**
 * @file ml_dsl_lexer.c
 * @brief ML DSL Lexer Implementation
 */

#include "ml_dsl_lexer.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

// Keyword mapping
typedef struct {
    const char* keyword;
    TokenType type;
} KeywordMapping;

static const KeywordMapping keywords[] = {
    {"ml", TOKEN_ML},
    {"model", TOKEN_MODEL},
    {"type", TOKEN_TYPE},
    {"input", TOKEN_INPUT},
    {"output", TOKEN_OUTPUT},
    {"train", TOKEN_TRAIN},
    {"predict", TOKEN_PREDICT},
    {"with", TOKEN_WITH},
    {"on", TOKEN_ON},
    {"dataset", TOKEN_DATASET},
    {"epochs", TOKEN_EPOCHS},
    {"batch_size", TOKEN_BATCH_SIZE},
    
    // Algorithm types
    {"linear_regression", TOKEN_LINEAR_REGRESSION},
    {"decision_tree", TOKEN_DECISION_TREE},
    {"svm", TOKEN_SVM},
    {"kmeans", TOKEN_KMEANS},
    {"qlearning", TOKEN_QLEARNING},
    
    // Parameters
    {"learning_rate", TOKEN_LEARNING_RATE},
    {"max_depth", TOKEN_MAX_DEPTH},
    {"min_samples_split", TOKEN_MIN_SAMPLES_SPLIT},
    {"criterion", TOKEN_CRITERION},
    {"kernel", TOKEN_KERNEL},
    {"C", TOKEN_C_PARAM},
    {"gamma", TOKEN_GAMMA},
    {"n_clusters", TOKEN_N_CLUSTERS},
    {"max_iterations", TOKEN_MAX_ITERATIONS},
    {"state_space", TOKEN_STATE_SPACE},
    {"action_space", TOKEN_ACTION_SPACE},
    {"discount_factor", TOKEN_DISCOUNT_FACTOR},
    {"epsilon", TOKEN_EPSILON},
    
    // Data types
    {"vector", TOKEN_VECTOR},
    {"scalar", TOKEN_SCALAR},
    {"class", TOKEN_CLASS},
    {"cluster", TOKEN_CLUSTER},
    {"discrete", TOKEN_DISCRETE},
    
    // Loss functions
    {"mse", TOKEN_MSE},
    {"gini", TOKEN_GINI},
    {"entropy", TOKEN_ENTROPY},
    
    // Optimizers
    {"gradient_descent", TOKEN_GRADIENT_DESCENT},
    {"sgd", TOKEN_SGD},
    {"adam", TOKEN_ADAM},
    
    // Kernels
    {"linear", TOKEN_LINEAR},
    {"rbf", TOKEN_RBF},
    {"poly", TOKEN_POLY},
    
    {nullptr, TOKEN_UNKNOWN}
};

// Helper functions

static bool is_at_end(const Lexer* lexer) {
    return lexer->current >= lexer->source_length;
}

static char peek(const Lexer* lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->source[lexer->current];
}

static char peek_next(const Lexer* lexer) {
    if (lexer->current + 1 >= lexer->source_length) return '\0';
    return lexer->source[lexer->current + 1];
}

static char advance(Lexer* lexer) {
    if (is_at_end(lexer)) return '\0';
    
    char c = lexer->source[lexer->current++];
    
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    
    return c;
}

static bool match(Lexer* lexer, char expected) {
    if (is_at_end(lexer)) return false;
    if (lexer->source[lexer->current] != expected) return false;
    
    advance(lexer);
    return true;
}

static void skip_whitespace(Lexer* lexer) {
    while (!is_at_end(lexer)) {
        char c = peek(lexer);
        
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance(lexer);
        } else if (c == '/' && peek_next(lexer) == '/') {
            // Skip line comment
            while (!is_at_end(lexer) && peek(lexer) != '\n') {
                advance(lexer);
            }
        } else if (c == '/' && peek_next(lexer) == '*') {
            // Skip block comment
            advance(lexer); // /
            advance(lexer); // *
            
            while (!is_at_end(lexer)) {
                if (peek(lexer) == '*' && peek_next(lexer) == '/') {
                    advance(lexer); // *
                    advance(lexer); // /
                    break;
                }
                advance(lexer);
            }
        } else {
            break;
        }
    }
}

static Token make_token(Lexer* lexer, TokenType type) {
    Token token;
    token.type = type;
    token.length = lexer->current - lexer->start;
    token.lexeme = strndup(&lexer->source[lexer->start], token.length);
    token.line = lexer->line;
    token.column = lexer->column - token.length;
    token.value.number = 0.0;
    token.value.string = nullptr;
    
    return token;
}

static Token make_error_token(Lexer* lexer, const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.lexeme = strdup(message);
    token.length = strlen(message);
    token.line = lexer->line;
    token.column = lexer->column;
    token.value.number = 0.0;
    token.value.string = nullptr;
    
    lexer_report_error(lexer, message);
    
    return token;
}

static TokenType check_keyword(const char* lexeme, size_t length) {
    for (size_t i = 0; keywords[i].keyword != nullptr; i++) {
        if (strlen(keywords[i].keyword) == length &&
            strncmp(keywords[i].keyword, lexeme, length) == 0) {
            return keywords[i].type;
        }
    }
    return TOKEN_IDENTIFIER;
}

static Token scan_identifier(Lexer* lexer) {
    while (isalnum(peek(lexer)) || peek(lexer) == '_') {
        advance(lexer);
    }
    
    size_t length = lexer->current - lexer->start;
    TokenType type = check_keyword(&lexer->source[lexer->start], length);
    
    return make_token(lexer, type);
}

static Token scan_number(Lexer* lexer) {
    while (isdigit(peek(lexer))) {
        advance(lexer);
    }
    
    // Check for decimal point
    if (peek(lexer) == '.' && isdigit(peek_next(lexer))) {
        advance(lexer); // consume '.'
        
        while (isdigit(peek(lexer))) {
            advance(lexer);
        }
    }
    
    // Check for scientific notation
    if (peek(lexer) == 'e' || peek(lexer) == 'E') {
        advance(lexer);
        
        if (peek(lexer) == '+' || peek(lexer) == '-') {
            advance(lexer);
        }
        
        while (isdigit(peek(lexer))) {
            advance(lexer);
        }
    }
    
    Token token = make_token(lexer, TOKEN_NUMBER);
    
    // Parse number value
    char* endptr;
    token.value.number = strtod(token.lexeme, &endptr);
    
    return token;
}

static Token scan_string(Lexer* lexer) {
    while (!is_at_end(lexer) && peek(lexer) != '"') {
        if (peek(lexer) == '\\') {
            advance(lexer); // escape character
            if (!is_at_end(lexer)) {
                advance(lexer); // escaped character
            }
        } else {
            advance(lexer);
        }
    }
    
    if (is_at_end(lexer)) {
        return make_error_token(lexer, "Unterminated string");
    }
    
    advance(lexer); // closing "
    
    Token token = make_token(lexer, TOKEN_STRING);
    
    // Extract string value (without quotes)
    size_t str_len = token.length - 2;
    token.value.string = strndup(&lexer->source[lexer->start + 1], str_len);
    
    return token;
}

// Lexer lifecycle

Lexer* lexer_create(const char* source) {
    if (!source) return nullptr;
    
    Lexer* lexer = malloc(sizeof(Lexer));
    if (!lexer) return nullptr;
    
    lexer->source = source;
    lexer->source_length = strlen(source);
    lexer->current = 0;
    lexer->start = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->has_error = false;
    lexer->error_message = nullptr;
    lexer->error_line = 0;
    lexer->error_column = 0;
    
    return lexer;
}

void lexer_destroy(Lexer* lexer) {
    if (!lexer) return;
    
    free(lexer->error_message);
    free(lexer);
}

// Tokenization

Token lexer_next_token(Lexer* lexer) {
    if (!lexer) {
        Token token = {.type = TOKEN_ERROR};
        return token;
    }
    
    skip_whitespace(lexer);
    
    lexer->start = lexer->current;
    
    if (is_at_end(lexer)) {
        return make_token(lexer, TOKEN_EOF);
    }
    
    char c = advance(lexer);
    
    // Identifiers and keywords
    if (isalpha(c) || c == '_') {
        return scan_identifier(lexer);
    }
    
    // Numbers
    if (isdigit(c)) {
        return scan_number(lexer);
    }
    
    // String literals
    if (c == '"') {
        return scan_string(lexer);
    }
    
    // Single-character tokens
    switch (c) {
        case ':': return make_token(lexer, TOKEN_COLON);
        case ';': return make_token(lexer, TOKEN_SEMICOLON);
        case ',': return make_token(lexer, TOKEN_COMMA);
        case '{': return make_token(lexer, TOKEN_LBRACE);
        case '}': return make_token(lexer, TOKEN_RBRACE);
        case '[': return make_token(lexer, TOKEN_LBRACKET);
        case ']': return make_token(lexer, TOKEN_RBRACKET);
        case '(': return make_token(lexer, TOKEN_LPAREN);
        case ')': return make_token(lexer, TOKEN_RPAREN);
        case '.': return make_token(lexer, TOKEN_DOT);
        case '=': return make_token(lexer, TOKEN_ASSIGN);
    }
    
    char error_msg[64];
    snprintf(error_msg, sizeof(error_msg), "Unexpected character: '%c'", c);
    return make_error_token(lexer, error_msg);
}

Token* lexer_tokenize_all(Lexer* lexer, size_t* out_count) {
    if (!lexer || !out_count) return nullptr;
    
    size_t capacity = 128;
    size_t count = 0;
    Token* tokens = malloc(capacity * sizeof(Token));
    
    if (!tokens) return nullptr;
    
    while (true) {
        Token token = lexer_next_token(lexer);
        
        if (count >= capacity) {
            capacity *= 2;
            Token* new_tokens = realloc(tokens, capacity * sizeof(Token));
            if (!new_tokens) {
                free(tokens);
                return nullptr;
            }
            tokens = new_tokens;
        }
        
        tokens[count++] = token;
        
        if (token.type == TOKEN_EOF || token.type == TOKEN_ERROR) {
            break;
        }
    }
    
    *out_count = count;
    return tokens;
}

// Token utilities

const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_ML: return "ML";
        case TOKEN_MODEL: return "MODEL";
        case TOKEN_TYPE: return "TYPE";
        case TOKEN_INPUT: return "INPUT";
        case TOKEN_OUTPUT: return "OUTPUT";
        case TOKEN_TRAIN: return "TRAIN";
        case TOKEN_PREDICT: return "PREDICT";
        case TOKEN_WITH: return "WITH";
        case TOKEN_ON: return "ON";
        case TOKEN_LINEAR_REGRESSION: return "LINEAR_REGRESSION";
        case TOKEN_DECISION_TREE: return "DECISION_TREE";
        case TOKEN_SVM: return "SVM";
        case TOKEN_KMEANS: return "KMEANS";
        case TOKEN_QLEARNING: return "QLEARNING";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_STRING: return "STRING";
        case TOKEN_COLON: return "COLON";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void token_destroy(Token* token) {
    if (!token) return;
    
    free(token->lexeme);
    if (token->type == TOKEN_STRING) {
        free(token->value.string);
    }
}

Token token_copy(const Token* token) {
    if (!token) {
        Token empty = {.type = TOKEN_ERROR};
        return empty;
    }
    
    Token copy = *token;
    copy.lexeme = token->lexeme ? strdup(token->lexeme) : nullptr;
    
    if (token->type == TOKEN_STRING && token->value.string) {
        copy.value.string = strdup(token->value.string);
    }
    
    return copy;
}

void token_print(const Token* token) {
    if (!token) return;
    
    printf("Token(%s, '%s', line=%zu, col=%zu)",
           token_type_to_string(token->type),
           token->lexeme ? token->lexeme : "",
           token->line,
           token->column);
    
    if (token->type == TOKEN_NUMBER) {
        printf(" [value=%.2f]", token->value.number);
    } else if (token->type == TOKEN_STRING) {
        printf(" [value=\"%s\"]", token->value.string ? token->value.string : "");
    }
    
    printf("\n");
}

// Error handling

bool lexer_has_error(const Lexer* lexer) {
    return lexer ? lexer->has_error : false;
}

const char* lexer_get_error(const Lexer* lexer) {
    return lexer ? lexer->error_message : nullptr;
}

void lexer_report_error(Lexer* lexer, const char* message) {
    if (!lexer) return;
    
    lexer->has_error = true;
    lexer->error_line = lexer->line;
    lexer->error_column = lexer->column;
    
    free(lexer->error_message);
    lexer->error_message = message ? strdup(message) : nullptr;
}
