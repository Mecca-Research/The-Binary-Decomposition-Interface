// ===================================================================
// DESC: Implementation of the Symbol Table for managing scopes and
//       identifiers.
// ===================================================================

#include "c23_compat.h"
#include "bci_symbol.h"
#include <stdlib.h>
#include <string.h>

// --- Symbol Table Public API Implementation ---

void symbol_table_init(SymbolTable* table) {
    if (!table) return;
    bci_vec_init(&table->symbols);
    table->scope_depth = 0;
}

void symbol_table_free(SymbolTable* table) {
    if (!table) return;
    bci_vec_free(&table->symbols);
}

void symbol_table_begin_scope(SymbolTable* table) {
    table->scope_depth++;
}

void symbol_table_end_scope(SymbolTable* table) {
    while (table->symbols.len > 0 &&
           table->symbols.data[table->symbols.len - 1].depth > table->scope_depth - 1) {
        // "Pop" the symbol by reducing the length of the vector.
        table->symbols.len--;
    }
    table->scope_depth--;
}

bool symbol_table_add(SymbolTable* table, Symbol symbol) {
    // Check for redeclaration in the same scope.
    for (int i = table->symbols.len - 1; i >= 0; i--) {
        Symbol* existing = &table->symbols.data[i];
        if (existing->depth < table->scope_depth) {
            break; // We've left the current scope.
        }
        if (strncmp(existing->name.start, symbol.name.start, symbol.name.length) == 0 &&
            strlen(existing->name.start) == symbol.name.length) {
            // Error: Symbol with same name already declared in this scope.
            return false;
        }
    }

    symbol.depth = table->scope_depth;
    bci_vec_push(&table->symbols, symbol);
    return true;
}
[[nodiscard]] 
Symbol* symbol_table_lookup(SymbolTable* table, Token name) {
    // Search backwards to find the innermost declaration (shadowing).
    for (int i = table->symbols.len - 1; i >= 0; i--) {
        Symbol* symbol = &table->symbols.data[i];
        if (name.length == symbol->name.length &&
            memcmp(name.start, symbol->name.start, name.length) == 0) {
            return symbol;
        }
    }
    return nullptr;
}
