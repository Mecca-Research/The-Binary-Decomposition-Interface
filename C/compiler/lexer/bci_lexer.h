// ===================================================================
// DESC: Defines the Lexer, which is responsible for scanning source
//       code and converting it into a stream of tokens.
// ===================================================================
#ifndef BCI_LEXER_H
#define BCI_LEXER_H

#include "bci_token.h"

// --- Lexer Structure ---
// Holds the state of the scanner as it reads through the source code.
typedef struct {
    const char* start;      // Start of the current lexeme being scanned.
    const char* current;    // The current character being looked at.
    int line;               // The current line number for error reporting.
} Lexer;

// --- Lexer Public API ---

// Initializes the lexer with a source code string.
void lexer_init(Lexer* lexer, const char* source);

// Scans and returns the next token from the source code.
Token lexer_scan_token(Lexer* lexer);

#endif // BCI_LEXER_H
