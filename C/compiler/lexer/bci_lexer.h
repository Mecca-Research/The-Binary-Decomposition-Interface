// ===================================================================
// DESC: Defines the Lexer, which is responsible for scanning source
//       code and converting it into a stream of tokens.
// ===================================================================
#ifndef BCI_LEXER_H
#define BCI_LEXER_H

#include "c23_compat.h"
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


// Compile-time invariants
static_assert(sizeof(void*) >= 4, "Lexer requires at least 32-bit pointers");
static_assert(sizeof(char) == 1, "char must be 1 byte");
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");

#endif // BCI_LEXER_H
