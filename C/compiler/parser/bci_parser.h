// ===================================================================
// DESC: Defines the Parser, which consumes a stream of tokens from
//       the Lexer to build an Abstract Syntax Tree (AST).
// ===================================================================
#ifndef BCI_PARSER_H
#define BCI_PARSER_H

#include "bci_lexer.h"
#include "bci_ast.h"

// --- Parser Structure ---
// Holds the state of the parser as it consumes tokens.
typedef struct {
    Lexer* lexer;       // Pointer to the lexer providing the token stream.
    Token current;      // The current token being processed.
    Token previous;     // The most recently consumed token.
    bool had_error;     // Flag set if a syntax error is found.
    bool panic_mode;    // Flag to prevent cascading error messages.
} Parser;


// --- Parser Public API ---

// Initializes the parser with a pointer to a lexer.
void parser_init(Parser* parser, Lexer* lexer);

// Frees any resources held by the parser.
void parser_free(Parser* parser);

// The main entry point for parsing. It consumes tokens from the lexer
// and returns the root of the constructed Abstract Syntax Tree.
// Returns NULL if a fatal syntax error occurs.
AstNode* parser_parse(Parser* parser);


#endif // BCI_PARSER_H
