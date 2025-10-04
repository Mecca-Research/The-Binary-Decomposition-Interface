
// ===================================================================
// DESC: Implementation of Extended Parser for Phase 3
// ===================================================================

#include "c23_compat.h"
#include "bci_parser_extended.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// --- Forward Declarations ---
static void init_parse_rules(ParserExtended* parser);
static AstNode* parse_number(Parser* parser);
static AstNode* parse_grouping(Parser* parser);
static AstNode* parse_unary_prefix(Parser* parser);
static AstNode* parse_binary_infix(Parser* parser, AstNode* left);

// --- Parser Extended Initialization ---

void parser_extended_init(ParserExtended* parser, Lexer* lexer) {
    parser_init(&parser->base, lexer);
    bci_vec_init(&parser->error_recovery.errors);
    parser->error_recovery.in_panic_mode = false;
    parser->rules = nullptr;
    parser->rule_count = 0;
    init_parse_rules(parser);
}

void parser_extended_free(ParserExtended* parser) {
    if (!parser) return;
    
    for (size_t i = 0; i < parser->error_recovery.errors.len; i++) {
        free((void*)parser->error_recovery.errors.data[i].message);
    }
    bci_vec_free(&parser->error_recovery.errors);
    
    free(parser->rules);
    parser_free(&parser->base);
}

// --- Parse Rules Initialization ---

static void init_parse_rules(ParserExtended* parser) {
    // Allocate space for all token types
    parser->rule_count = 64; // Adjust based on token types
    parser->rules = calloc(parser->rule_count, sizeof(ParseRule));
    
    // Initialize rules for different token types
    // Numbers
    parser->rules[0].prefix = parse_number;
    parser->rules[0].infix = nullptr;
    parser->rules[0].precedence = PREC_NONE;
    
    // Grouping (parentheses)
    parser->rules[1].prefix = parse_grouping;
    parser->rules[1].infix = nullptr;
    parser->rules[1].precedence = PREC_NONE;
    
    // Unary operators
    parser->rules[2].prefix = parse_unary_prefix;
    parser->rules[2].infix = nullptr;
    parser->rules[2].precedence = PREC_UNARY;
    
    // Binary operators
    parser->rules[3].prefix = nullptr;
    parser->rules[3].infix = parse_binary_infix;
    parser->rules[3].precedence = PREC_TERM;
}

ParseRule* get_rule(ParserExtended* parser, TokenKind type) {
    if (type >= parser->rule_count) {
        return &parser->rules[0]; // Default rule
    }
    return &parser->rules[type];
}

// --- Error Recovery ---

void parser_error_at(ParserExtended* parser, Token* token, const char* message) {
    if (parser->error_recovery.in_panic_mode) return;
    
    parser->error_recovery.in_panic_mode = true;
    parser->base.had_error = true;
    
    ParseError error;
    error.message = strdup(message);
    error.file = nullptr; // Token doesn't have file field in base implementation
    error.line = token->line;
    error.column = 0; // Could be enhanced
    
    bci_vec_push(&parser->error_recovery.errors, error);
    
    fprintf(stderr, "[line %d] Error: %s\n", token->line, message);
}

void parser_synchronize(ParserExtended* parser) {
    parser->error_recovery.in_panic_mode = false;
    
    // Skip tokens until we find a statement boundary
    while (parser->base.current.kind != TOKEN_EOF) {
        if (parser->base.previous.kind == TOKEN_SEMICOLON) return;
        
        switch (parser->base.current.kind) {
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_FOR:
            case TOKEN_RETURN:
                return;
            default:
                ; // Continue
        }
        
        // Advance to next token - this is critical to avoid infinite loop
        parser->base.previous = parser->base.current;
        parser->base.current = lexer_scan_token(parser->base.lexer);
    }
}

void parser_report_errors(ParserExtended* parser) {
    if (parser->error_recovery.errors.len == 0) {
        printf("No parse errors.\n");
        return;
    }
    
    printf("Parse errors (%zu):\n", parser->error_recovery.errors.len);
    for (size_t i = 0; i < parser->error_recovery.errors.len; i++) {
        ParseError* err = &parser->error_recovery.errors.data[i];
        printf("  [%s:%d] %s\n", err->file ? err->file : "unknown", 
               err->line, err->message);
    }
}

// --- Precedence Parsing ---

AstNode* parse_precedence(ParserExtended* parser, Precedence precedence) {
    // Get prefix rule for current token
    ParseRule* rule = get_rule(parser, parser->base.current.kind);
    ParsePrefixFn prefix = rule->prefix;
    
    if (!prefix) {
        parser_error_at(parser, &parser->base.current, "Expected expression");
        return nullptr;
    }
    
    // Advance to consume the token before calling prefix function
    // This is critical - without this, the parser never advances and loops forever
    parser->base.previous = parser->base.current;
    parser->base.current = lexer_scan_token(parser->base.lexer);
    
    AstNode* left = prefix(&parser->base);
    
    // Parse infix operators with higher precedence
    while (precedence <= get_rule(parser, parser->base.current.kind)->precedence) {
        rule = get_rule(parser, parser->base.current.kind);
        ParseInfixFn infix = rule->infix;
        if (!infix) break;
        
        // Advance to consume the operator token before calling infix function
        parser->base.previous = parser->base.current;
        parser->base.current = lexer_scan_token(parser->base.lexer);
        
        left = infix(&parser->base, left);
    }
    
    return left;
}

// --- Expression Parsing ---

AstNode* parse_expression(ParserExtended* parser) {
    return parse_precedence(parser, PREC_ASSIGNMENT);
}

static AstNode* parse_number(Parser* parser) {
    // Consume the number token
    Token number_token = parser->previous;
    
    // In the base parser, advance() is called before prefix functions
    // But we need to ensure we've consumed the token for the extended parser
    // The token is already in 'previous' from the precedence parser
    
    // Create AST node from the token value
    long long value = strtoll(number_token.start, nullptr, 10);
    return ast_new_literal_int(value);
}

static AstNode* parse_grouping(Parser* parser) {
    (void)parser;
    // Simplified: parse expression between parentheses
    return nullptr;
}

static AstNode* parse_unary_prefix(Parser* parser) {
    (void)parser;
    // Simplified: parse unary operator
    return nullptr;
}

static AstNode* parse_binary_infix(Parser* parser, AstNode* left) {
    (void)parser;
    (void)left;
    // Simplified: parse binary operator
    return nullptr;
}

AstNode* parse_primary(ParserExtended* parser) {
    return parse_precedence(parser, PREC_PRIMARY);
}

AstNode* parse_unary(ParserExtended* parser) {
    return parse_precedence(parser, PREC_UNARY);
}

AstNode* parse_binary(ParserExtended* parser, AstNode* left) {
    (void)parser;
    (void)left;
    return nullptr;
}

AstNode* parse_call(ParserExtended* parser, AstNode* callee) {
    (void)parser;
    (void)callee;
    return nullptr;
}

// --- Pattern Matching ---

AstNode* parse_match_expr(ParserExtended* parser) {
    (void)parser;
    // Simplified implementation
    AstNode* node = ast_new_node(AST_NODE_MATCH_EXPR);
    return node;
}

AstNode* parse_pattern(ParserExtended* parser) {
    (void)parser;
    // Simplified implementation
    AstNode* node = ast_new_node(AST_NODE_PATTERN);
    return node;
}

// --- Lambda Expressions ---

AstNode* parse_lambda(ParserExtended* parser) {
    (void)parser;
    // Simplified implementation
    AstNode* node = ast_new_node(AST_NODE_LAMBDA);
    return node;
}

// --- Main Parse Entry Point ---

AstNode* parser_extended_parse(ParserExtended* parser) {
    AstNode* program = ast_new_program();
    
    // Parse until EOF
    while (parser->base.current.kind != TOKEN_EOF) {
        AstNode* stmt = parse_expression(parser);
        if (stmt) {
            // Add to program (simplified)
        }
        
        if (parser->base.had_error) {
            parser_synchronize(parser);
        }
    }
    
    return program;
}
