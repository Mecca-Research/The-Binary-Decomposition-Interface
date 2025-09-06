// ===================================================================
// DESC: Implementation of the Parser.
// ===================================================================

#include "bci_parser.h"
#include <stdio.h>
#include <stdlib.h>

// --- Forward Declarations for Grammar Rules ---
static AstNode* expression(Parser* parser);
static AstNode* statement(Parser* parser);
static AstNode* declaration(Parser* parser);

// --- Helper Functions ---

// Reports an error at a specific token's location.
static void error_at(Parser* parser, Token* token, const char* message) {
    if (parser->panic_mode) return;
    parser->panic_mode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->kind == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->kind == TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser->had_error = true;
}

// Reports an error at the parser's previous token.
static void error(Parser* parser, const char* message) {
    error_at(parser, &parser->previous, message);
}

// Reports an error at the parser's current token.
static void error_at_current(Parser* parser, const char* message) {
    error_at(parser, &parser->current, message);
}

// Consumes the current token and advances to the next one.
static void advance(Parser* parser) {
    parser->previous = parser->current;
    for (;;) {
        parser->current = lexer_scan_token(parser->lexer);
        if (parser->current.kind != TOKEN_ERROR) break;
        error_at_current(parser, parser->current.start);
    }
}

// Consumes the current token if it matches the expected kind.
// Reports an error otherwise.
static void consume(Parser* parser, TokenKind kind, const char* message) {
    if (parser->current.kind == kind) {
        advance(parser);
        return;
    }
    error_at_current(parser, message);
}

// Checks if the current token is of the given kind.
static bool check(Parser* parser, TokenKind kind) {
    return parser->current.kind == kind;
}

// If the current token matches the given kind, consumes it and returns true.
static bool match(Parser* parser, TokenKind kind) {
    if (!check(parser, kind)) return false;
    advance(parser);
    return true;
}

// --- Grammar Rule Implementations (Recursive Descent) ---

// Forward declaration for the precedence table.
typedef struct ParseRule ParseRule;
static ParseRule* get_rule(TokenKind kind);

// primary = NUMBER | STRING | "true" | "false" | "nil" | "(" expression ")" | IDENTIFIER ;
static AstNode* primary(Parser* parser) {
    if (match(parser, TOKEN_FALSE)) return ast_new_literal_int(0); // Representing bools as ints
    if (match(parser, TOKEN_TRUE)) return ast_new_literal_int(1);
    if (match(parser, TOKEN_NIL)) return ast_new_literal_int(0); // Or a specific nil type

    if (match(parser, TOKEN_INT_LITERAL)) {
        long long value = strtoll(parser->previous.start, NULL, 10);
        return ast_new_literal_int(value);
    }
    
    if (match(parser, TOKEN_FLOAT_LITERAL)) {
        // Note: ast_new_literal_int is a placeholder. A proper AST would have
        // AstNode* ast_new_literal_float(double value);
        // For now, we will truncate.
        double value = strtod(parser->previous.start, NULL);
        return ast_new_literal_int((int64_t)value);
    }

    if (match(parser, TOKEN_LPAREN)) {
        AstNode* expr = expression(parser);
        consume(parser, TOKEN_RPAREN, "Expect ')' after expression.");
        return expr;
    }

    error(parser, "Expect expression.");
    return NULL;
}


// unary = ( "!" | "-" ) unary | primary ;
static AstNode* unary(Parser* parser) {
    if (match(parser, TOKEN_BANG) || match(parser, TOKEN_MINUS)) {
        const char* op = parser->previous.kind == TOKEN_BANG ? "!" : "-";
        AstNode* right = unary(parser);
        // A proper AST would have ast_new_unary_op. We'll reuse binary for now.
        return ast_new_binary_op(op, NULL, right);
    }
    return primary(parser);
}

// factor = unary ( ( "/" | "*" ) unary )* ;
static AstNode* factor(Parser* parser) {
    AstNode* expr = unary(parser);
    while (match(parser, TOKEN_SLASH) || match(parser, TOKEN_STAR)) {
        const char* op = parser->previous.kind == TOKEN_SLASH ? "/" : "*";
        AstNode* right = unary(parser);
        expr = ast_new_binary_op(op, expr, right);
    }
    return expr;
}

// term = factor ( ( "-" | "+" ) factor )* ;
static AstNode* term(Parser* parser) {
    AstNode* expr = factor(parser);
    while (match(parser, TOKEN_MINUS) || match(parser, TOKEN_PLUS)) {
        const char* op = parser->previous.kind == TOKEN_MINUS ? "-" : "+";
        AstNode* right = factor(parser);
        expr = ast_new_binary_op(op, expr, right);
    }
    return expr;
}

// comparison = term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
static AstNode* comparison(Parser* parser) {
    AstNode* expr = term(parser);
    while (match(parser, TOKEN_GREATER) || match(parser, TOKEN_GREATER_EQUAL) ||
           match(parser, TOKEN_LESS) || match(parser, TOKEN_LESS_EQUAL)) {
        const char* op;
        switch (parser->previous.kind) {
            case TOKEN_GREATER:       op = ">"; break;
            case TOKEN_GREATER_EQUAL: op = ">="; break;
            case TOKEN_LESS:          op = "<"; break;
            case TOKEN_LESS_EQUAL:    op = "<="; break;
            default: op = ""; // Unreachable
        }
        AstNode* right = term(parser);
        expr = ast_new_binary_op(op, expr, right);
    }
    return expr;
}

// equality = comparison ( ( "!=" | "==" ) comparison )* ;
static AstNode* equality(Parser* parser) {
    AstNode* expr = comparison(parser);
    while (match(parser, TOKEN_BANG_EQUAL) || match(parser, TOKEN_EQUAL_EQUAL)) {
        const char* op = parser->previous.kind == TOKEN_BANG_EQUAL ? "!=" : "==";
        AstNode* right = comparison(parser);
        expr = ast_new_binary_op(op, expr, right);
    }
    return expr;
}

// expression = equality ; (For now, lowest precedence)
static AstNode* expression(Parser* parser) {
    return equality(parser);
}

// statement = exprStmt | printStmt | block ;
static AstNode* statement(Parser* parser) {
    // A proper implementation would have print statements, etc.
    // For now, all statements are expression statements.
    AstNode* expr = expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after expression.");
    return expr;
}

// declaration = varDecl | statement ;
static AstNode* declaration(Parser* parser) {
    // A proper implementation would parse variable/function declarations.
    // We'll add this logic in later stages.
    return statement(parser);
}

// Tries to recover from a syntax error by discarding tokens until a likely
// statement boundary is found.
static void synchronize(Parser* parser) {
    parser->panic_mode = false;
    while (parser->current.kind != TOKEN_EOF) {
        if (parser->previous.kind == TOKEN_SEMICOLON) return;
        switch (parser->current.kind) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default:
                ; // Do nothing.
        }
        advance(parser);
    }
}

// --- Parser Public API Implementation ---

void parser_init(Parser* parser, Lexer* lexer) {
    parser->lexer = lexer;
    parser->had_error = false;
    parser->panic_mode = false;
    // Prime the parser with the first two tokens.
    advance(parser);
}

void parser_free(Parser* parser) {
    // Nothing to free, parser doesn't own the lexer.
}

AstNode* parser_parse(Parser* parser) {
    AstNode* program = ast_new_program();
    if (!program) return NULL;

    while (!match(parser, TOKEN_EOF)) {
        AstNode* decl = declaration(parser);
        if (decl) {
            bci_vec_push(program->as.block, decl);
        }
        if (parser->panic_mode) synchronize(parser);
    }

    return parser->had_error ? NULL : program;
}
