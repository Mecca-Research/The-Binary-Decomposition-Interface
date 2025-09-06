// ===================================================================
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
