// ===================================================================
// DESC: Defines the Symbol and SymbolTable data structures, used by
//       the Semantic Analyzer to track declared identifiers like
//       variables and functions.
// ===================================================================
#ifndef BCI_SYMBOL_H
#define BCI_SYMBOL_H

#include "bci_types.h"
#include "bci_token.h"

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
// Returns a pointer to the Symbol if found, otherwise NULL.
Symbol* symbol_table_lookup(SymbolTable* table, Token name);

#endif // BCI_SYMBOL_H


// ===================================================================
// FILE: bci_analyzer.h
// DESC: Defines the Semantic Analyzer, which traverses the AST to
//       perform semantic checks like type checking and scope resolution.
// ===================================================================
#ifndef BCI_ANALYZER_H
#define BCI_ANALYZER_H

#include "bci_ast.h"
#include "bci_symbol.h"
#include <stdbool.h>

// --- Analyzer Structure ---
// Holds the state for the semantic analysis pass.
typedef struct {
    SymbolTable* symbol_table;
    bool had_error;
} Analyzer;


// --- Analyzer Public API ---

// Initializes a new semantic analyzer.
void analyzer_init(Analyzer* analyzer);

// Frees the resources used by the analyzer, including its symbol table.
void analyzer_free(Analyzer* analyzer);

// The main entry point for semantic analysis. Traverses the program's
// AST, performs all semantic checks, and annotates the AST with type
// information. Returns true if the program is semantically valid.
bool analyzer_analyze(Analyzer* analyzer, AstNode* program);

#endif // BCI_ANALYZER_H
