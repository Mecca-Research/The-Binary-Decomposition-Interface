// ===================================================================
// DESC: Defines the Symbol and SymbolTable data structures, used by
//       the Semantic Analyzer to track declared identifiers like
//       variables and functions.
// ===================================================================
#ifndef BCI_SYMBOL_H
#define BCI_SYMBOL_H

#include "c23_compat.h"
#include "../types/bci_types.h"
#include "../lexer/bci_token.h"

// --- Symbol Structure ---
// Stores information about a declared variable or function.
typedef struct {
    Token name;        // The token containing the identifier's name.
    BciType* type;     // The type of the symbol.
    int depth;         // The scope depth where this symbol was declared.
} Symbol;

// --- Symbol Table Structure ---
// A stack-based symbol table to manage scopes.
typedef struct {
    BciVec(Symbol) symbols;     // A dynamic array of all active symbols.
    int scope_depth;            // The current nesting level of scopes.
} SymbolTable;


// --- Symbol Table Public API ---

// Initializes a symbol table.
void symbol_table_init(SymbolTable* table);

// Frees all resources used by a symbol table.
void symbol_table_free(SymbolTable* table);

// Enters a new scope.
void symbol_table_begin_scope(SymbolTable* table);

// Exits the current scope, removing all symbols declared within it.
void symbol_table_end_scope(SymbolTable* table);

// Adds a new symbol to the current scope. Returns false if redeclared.
bool symbol_table_add(SymbolTable* table, Symbol symbol);

// Looks up a symbol by name, searching from the innermost scope outwards.
// Returns a pointer to the Symbol if found, otherwise nullptr.
[[nodiscard]] Symbol* symbol_table_lookup(SymbolTable* table, Token name);


// Compile-time invariants
static_assert(sizeof(void*) >= 4, "Symbol table requires at least 32-bit pointers");
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");

#endif // BCI_SYMBOL_H
