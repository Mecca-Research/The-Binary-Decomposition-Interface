// ===================================================================
// DESC: Implementation of the AI Training Engine.
// ===================================================================

#include "c23_compat.h"
#include "ai_trainer.h"
#include <stdio.h>

// --- Private Helper to Load the Curriculum ---

// This function defines the AI's initial knowledge base according
// to the "liked/unliked pairs" and "conflict resolution" model.
static void load_curriculum(TrainingTable* table) {
    printf("Loading AI curriculum...\n");

    // Phase 1: Basic Arithmetic (as per your design)
    training_table_add_rule(table, (TrainingRule){
        {VAL_KIND_INT, .as.int_val = 1}, {VAL_KIND_INT, .as.int_val = 1},
        OP_KIND_ADD, {VAL_KIND_INT, .as.int_val = 2},
        CONFLICT_TYPE_UNLIKED_PAIR, "{2}"
    });
    training_table_add_rule(table, (TrainingRule){
        {VAL_KIND_INT, .as.int_val = 2}, {VAL_KIND_INT, .as.int_val = 1},
        OP_KIND_SUBTRACT, {VAL_KIND_INT, .as.int_val = 1},
        CONFLICT_TYPE_STABLE, "{1}"
    });
    training_table_add_rule(table, (TrainingRule){
        {VAL_KIND_INT, .as.int_val = 3}, {VAL_KIND_INT, .as.int_val = 2},
        OP_KIND_MULTIPLY, {VAL_KIND_INT, .as.int_val = 6},
        CONFLICT_TYPE_UNLIKED_PAIR, "{6}" // Assuming new module needed
    });
    training_table_add_rule(table, (TrainingRule){
        {VAL_KIND_INT, .as.int_val = 1}, {VAL_KIND_INT, .as.int_val = 0},
        OP_KIND_DIVIDE, {VAL_KIND_UNDEFINED},
        CONFLICT_TYPE_UNDEFINED, "Undefined Conflict"
    });
    
    // Add more rules here from your tables...
    printf("Curriculum loaded with %zu rules.\n\n", table->count);
}

// --- Public API Implementation ---

void ai_trainer_init(AITrainer* trainer) {
    if (!trainer) return;
    training_table_init(&trainer->curriculum);
    load_curriculum(&trainer->curriculum);
}

void ai_trainer_free(AITrainer* trainer) {
    if (!trainer) return;
    training_table_free(&trainer->curriculum);
}

// This is the main training loop. In a real system, this would feed
// the rules into a learning model. For our C program, it will simply
// process and print the rules to demonstrate the logic.
void ai_trainer_run(AITrainer* trainer) {
    if (!trainer) return;

    printf("--- Starting AI Training Session ---\n");
    for (size_t i = 0; i < trainer->curriculum.count; ++i) {
        TrainingRule* rule = &trainer->curriculum.rules[i];

        // This simulates the AI processing one rule.
        printf("Rule %zu: ", i + 1);

        const char* op_symbol;
        switch (rule->operation) {
            case OP_KIND_ADD:      op_symbol = "+"; break;
            case OP_KIND_SUBTRACT: op_symbol = "-"; break;
            case OP_KIND_MULTIPLY: op_symbol = "*"; break;
            case OP_KIND_DIVIDE:   op_symbol = "/"; break;
        }

        printf("Processing (%lld %s %lld) -> ",
               rule->input_a.as.int_val,
               op_symbol,
               rule->input_b.as.int_val);

        if (rule->result.kind == VAL_KIND_INT) {
            printf("Result: %lld. ", rule->result.as.int_val);
        } else {
            printf("Result: Undefined. ");
        }

        switch (rule->conflict) {
            case CONFLICT_TYPE_STABLE:
                printf("Type: Stable (Liked Pair). Reinforces module %s.\n", rule->resolved_module);
                break;
            case CONFLICT_TYPE_UNLIKED_PAIR:
                printf("Type: Conflict (Unliked Pair). Resolution creates module %s.\n", rule->resolved_module);
                break;
            case CONFLICT_TYPE_UNDEFINED:
                printf("Type: Critical Conflict. Resolution: %s.\n", rule->resolved_module);
                break;
        }
    }
    printf("--- AI Training Session Complete ---\n");
}
