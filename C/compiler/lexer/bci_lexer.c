// ===================================================================
// DESC: Implementation of the Lexer (Scanner).
// ===================================================================

#include "c23_compat.h"
#include "bci_lexer.h"
#include <string.h>
#include <stdbool.h>

// --- Helper Functions ---

// Checks if the lexer has reached the end of the source code.
static bool is_at_end(Lexer* lexer) {
    return *lexer->current == '\0';
}

// Consumes and returns the current character, advancing the lexer.
static char advance(Lexer* lexer) {
    lexer->current++;
    return lexer->current[-1];
}

// Returns the current character without consuming it.
static char peek(Lexer* lexer) {
    return *lexer->current;
}

// Returns the next character without consuming it.
static char peek_next(Lexer* lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->current[1];
}

// If the current character matches 'expected', consumes it and returns true.
static bool match(Lexer* lexer, char expected) {
    if (is_at_end(lexer)) return false;
    if (*lexer->current != expected) return false;
    lexer->current++;
    return true;
}

// Creates a token of the given kind.
static Token make_token(Lexer* lexer, TokenKind kind) {
    Token token;
    token.kind = kind;
    token.start = lexer->start;
    token.length = (int)(lexer->current - lexer->start);
    token.line = lexer->line;
    return token;
}

// Creates an error token.
static Token error_token(Lexer* lexer, const char* message) {
    Token token;
    token.kind = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = lexer->line;
    return token;
}

// Skips whitespace and comments.
static void skip_whitespace(Lexer* lexer) {
    for (;;) {
        char c = peek(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(lexer);
                break;
            case '\n':
                lexer->line++;
                advance(lexer);
                break;
            case '/':
                if (peek_next(lexer) == '/') {
                    // A comment goes until the end of the line.
                    while (peek(lexer) != '\n' && !is_at_end(lexer)) advance(lexer);
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

// Checks if an identifier is a keyword.
static TokenKind check_keyword(Lexer* lexer, int start, int length, const char* rest, TokenKind kind) {
    if (lexer->current - lexer->start == start + length &&
        memcmp(lexer->start + start, rest, length) == 0) {
        return kind;
    }
    return TOKEN_IDENTIFIER;
}

// Determines if an identifier is a keyword or a user-defined name.
static TokenKind identifier_type(Lexer* lexer) {
    switch (lexer->start[0]) {
        case 'a': return check_keyword(lexer, 1, 2, "nd", TOKEN_AND);
        case 'c': return check_keyword(lexer, 1, 4, "lass", TOKEN_CLASS);
        case 'e': return check_keyword(lexer, 1, 3, "lse", TOKEN_ELSE);
        case 'f':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'a': return check_keyword(lexer, 2, 3, "lse", TOKEN_FALSE);
                    case 'o': return check_keyword(lexer, 2, 1, "r", TOKEN_FOR);
                    case 'u': return check_keyword(lexer, 2, 1, "n", TOKEN_FUN);
                }
            }
            break;
        case 'i': return check_keyword(lexer, 1, 1, "f", TOKEN_IF);
        case 'n': return check_keyword(lexer, 1, 2, "il", TOKEN_NIL);
        case 'o': return check_keyword(lexer, 1, 1, "r", TOKEN_OR);
        case 'p': return check_keyword(lexer, 1, 4, "rint", TOKEN_PRINT);
        case 'r': return check_keyword(lexer, 1, 5, "eturn", TOKEN_RETURN);
        case 's': return check_keyword(lexer, 1, 4, "uper", TOKEN_SUPER);
        case 't':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'h': return check_keyword(lexer, 2, 2, "is", TOKEN_THIS);
                    case 'r': return check_keyword(lexer, 2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
        case 'v': return check_keyword(lexer, 1, 2, "ar", TOKEN_VAR);
        case 'w': return check_keyword(lexer, 1, 4, "hile", TOKEN_WHILE);
    }
    return TOKEN_IDENTIFIER;
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
}

// Scans an identifier or keyword.
static Token identifier(Lexer* lexer) {
    while (is_alpha(peek(lexer)) || is_digit(peek(lexer))) advance(lexer);
    return make_token(lexer, identifier_type(lexer));
}

// Scans a number literal (integer or float).
static Token number(Lexer* lexer) {
    while (is_digit(peek(lexer))) advance(lexer);

    // Look for a fractional part.
    if (peek(lexer) == '.' && is_digit(peek_next(lexer))) {
        // Consume the ".".
        advance(lexer);
        while (is_digit(peek(lexer))) advance(lexer);
        return make_token(lexer, TOKEN_FLOAT_LITERAL);
    }

    return make_token(lexer, TOKEN_INT_LITERAL);
}

// Scans a string literal.
static Token string(Lexer* lexer) {
    while (peek(lexer) != '"' && !is_at_end(lexer)) {
        if (peek(lexer) == '\n') lexer->line++;
        advance(lexer);
    }

    if (is_at_end(lexer)) return error_token(lexer, "Unterminated string.");

    // The closing quote.
    advance(lexer);
    return make_token(lexer, TOKEN_STRING);
}


// --- Lexer Public API Implementation ---

void lexer_init(Lexer* lexer, const char* source) {
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
}

Token lexer_scan_token(Lexer* lexer) {
    skip_whitespace(lexer);
    lexer->start = lexer->current;

    if (is_at_end(lexer)) return make_token(lexer, TOKEN_EOF);

    char c = advance(lexer);

    if (is_alpha(c)) return identifier(lexer);
    if (is_digit(c)) return number(lexer);

    switch (c) {
        case '(': return make_token(lexer, TOKEN_LPAREN);
        case ')': return make_token(lexer, TOKEN_RPAREN);
        case '{': return make_token(lexer, TOKEN_LBRACE);
        case '}': return make_token(lexer, TOKEN_RBRACE);
        case '[': return make_token(lexer, TOKEN_LBRACKET);
        case ']': return make_token(lexer, TOKEN_RBRACKET);
        case ';': return make_token(lexer, TOKEN_SEMICOLON);
        case ',': return make_token(lexer, TOKEN_COMMA);
        case '.': return make_token(lexer, TOKEN_DOT);
        case '-': return make_token(lexer, TOKEN_MINUS);
        case '+': return make_token(lexer, TOKEN_PLUS);
        case '/': return make_token(lexer, TOKEN_SLASH);
        case '*': return make_token(lexer, TOKEN_STAR);
        case '!': return make_token(lexer, match(lexer, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=': return make_token(lexer, match(lexer, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<': return make_token(lexer, match(lexer, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>': return make_token(lexer, match(lexer, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '"': return string(lexer);
    }

    return error_token(lexer, "Unexpected character.");
}
