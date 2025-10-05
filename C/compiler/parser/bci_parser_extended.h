
// ===================================================================
// DESC: Extended Parser for Phase 3 - Enhanced AST and Error Recovery
//       Adds operator precedence, pattern matching, lambda expressions
// ===================================================================
/**
 * @file bci_parser_extended.h
 * @brief Parser Implementation
 * @details This file provides the bci parser extended functionality for the BDI system.
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef BCI_PARSER_EXTENDED_H
#define BCI_PARSER_EXTENDED_H

#include "c23_compat.h"
#include "bci_parser.h"
#include "../ast/bci_ast_extended.h"

// --- Operator Precedence Levels ---
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // ||
    PREC_AND,         // &&
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! - +
    PREC_CALL,        // . () []
    PREC_PRIMARY
} Precedence;

// --- Parse Rule Function Types ---
typedef AstNode* (*ParsePrefixFn)(Parser* parser);
typedef AstNode* (*ParseInfixFn)(Parser* parser, AstNode* left);

// --- Parse Rule ---
typedef struct {
    ParsePrefixFn prefix;
    ParseInfixFn infix;
    Precedence precedence;
} ParseRule;

// --- Parse Error ---
typedef struct {
    const char* message;
    const char* file;
    int line;
    int column;
} ParseError;

// --- Error Recovery ---
typedef struct {
    BciVec(ParseError) errors;
    bool in_panic_mode;
} ErrorRecovery;

// --- Extended Parser ---
typedef struct {
    Parser base;
    ErrorRecovery error_recovery;
    ParseRule* rules;
    size_t rule_count;
} ParserExtended;

// --- Parser Extended API ---

void parser_extended_init(ParserExtended* parser, Lexer* lexer);
void parser_extended_free(ParserExtended* parser);
[[nodiscard]] AstNode* parser_extended_parse(ParserExtended* parser);

// Operator precedence parsing
[[nodiscard]] AstNode* parse_precedence(ParserExtended* parser, Precedence precedence);
[[nodiscard]] ParseRule* get_rule(ParserExtended* parser, TokenKind type);

// Error recovery
void parser_error_at(ParserExtended* parser, Token* token, const char* message);
void parser_synchronize(ParserExtended* parser);
void parser_report_errors(ParserExtended* parser);

// Expression parsing
[[nodiscard]] AstNode* parse_expression(ParserExtended* parser);
[[nodiscard]] AstNode* parse_primary(ParserExtended* parser);
[[nodiscard]] AstNode* parse_unary(ParserExtended* parser);
[[nodiscard]] AstNode* parse_binary(ParserExtended* parser, AstNode* left);
[[nodiscard]] AstNode* parse_call(ParserExtended* parser, AstNode* callee);

// Pattern matching
[[nodiscard]] AstNode* parse_match_expr(ParserExtended* parser);
[[nodiscard]] AstNode* parse_pattern(ParserExtended* parser);

// Lambda expressions
[[nodiscard]] AstNode* parse_lambda(ParserExtended* parser);

// Compile-time invariants
static_assert(sizeof(void*) >= 4, "Extended parser requires at least 32-bit pointers");

#endif // BCI_PARSER_EXTENDED_H
