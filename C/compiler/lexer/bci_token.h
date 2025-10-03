// ===================================================================
// DESC: Defines the Token structure and the different kinds of tokens
//       that the Lexer can produce.
// ===================================================================
#include "c23_compat.h"
#ifndef BCI_TOKEN_H
#define BCI_TOKEN_H

// --- Token Kind Enumeration ---
// Represents all possible types of tokens in the language.
typedef enum {
    // Single-character tokens.
    TOKEN_LPAREN, TOKEN_RPAREN,
    TOKEN_LBRACE, TOKEN_RBRACE,
    TOKEN_LBRACKET, TOKEN_RBRACKET,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
    TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,

    // One or two character tokens.
    TOKEN_BANG, TOKEN_BANG_EQUAL,
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,

    // Literals.
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_INT_LITERAL, TOKEN_FLOAT_LITERAL,

    // Keywords.
    TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
    TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
    TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
    TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,

    // Special tokens.
    TOKEN_ERROR, TOKEN_EOF
} TokenKind;

// --- Token Structure ---
// Represents a single token scanned from the source code.
typedef struct {
    TokenKind kind;
    const char* start; // Pointer to the beginning of the lexeme in the source.
    int length;        // Length of the lexeme.
    int line;          // Line number where the token appears, for error reporting.
} Token;

// Compile-time invariants
static_assert(sizeof(int) >= 4, "Token types require at least 32-bit int");
static_assert(sizeof(Token) > 0, "Token must have non-zero size");
static_assert(sizeof(TokenKind) == sizeof(int), "TokenKind must be int-sized");

#endif // BCI_TOKEN_H
