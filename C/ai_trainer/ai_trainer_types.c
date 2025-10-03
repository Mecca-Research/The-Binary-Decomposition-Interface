// ===================================================================
// DESC: Implementation for managing the core AI training data
//       structures.
// ===================================================================

#include "c23_compat.h"
#include "ai_trainer_types.h"
#include <stdlib.h>
#include <stdio.h>

// --- Public API for Managing Training Data ---

void training_table_init(TrainingTable* table) {
    if (!table) return;
    table->rules = nullptr;
    table->count = 0;
    table->capacity = 0;
}

void training_table_free(TrainingTable* table) {
    if (!table) return;
    free(table->rules);
    training_table_init(table); // Reset to a clean, initialized state.
}

void training_table_add_rule(TrainingTable* table, TrainingRule rule) {
    if (!table) return;

    // Check if we need to grow the dynamic array.
    if (table->capacity < table->count + 1) {
        size_t old_capacity = table->capacity;
        table->capacity = old_capacity < 8 ? 8 : old_capacity * 2;
        table->rules = realloc(table->rules, table->capacity * sizeof(TrainingRule));

        if (table->rules == nullptr) {
            perror("Failed to reallocate training table");
            // In a real application, this should be handled more gracefully.
            exit(1);
        }
    }

    table->rules[table->count] = rule;
    table->count++;
}
