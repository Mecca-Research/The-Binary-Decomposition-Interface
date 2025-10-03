// ===================================================================
// DESC: Defines the core data structures for the AI training engine,
//       representing the structured rules of logic and arithmetic.
// ===================================================================
#ifndef AI_TRAINER_TYPES_H
#define AI_TRAINER_TYPES_H

#include "c23_compat.h"
#include <stdint.h>
#include <stddef.h>

// --- Enumerations for Training Rules ---

// Represents the kind of operation in a rule.
typedef enum {
    OP_KIND_ADD,
    OP_KIND_SUBTRACT,
    OP_KIND_MULTIPLY,
    OP_KIND_DIVIDE
} OpKind;

// Represents the conflict type of a rule. [cite: 235, 240]
typedef enum {
    CONFLICT_TYPE_STABLE,       // A "liked pair" that reinforces identity. [cite: 209, 240]
    CONFLICT_TYPE_UNLIKED_PAIR, // A conflict that requires resolution. [cite: 210, 240]
    CONFLICT_TYPE_UNDEFINED     // A critical conflict, like division by zero. [cite: 240]
} ConflictType;


// --- Tagged Union for Rule Values ---
// Can hold either an integer value or an "Undefined" state.

typedef enum {
    VAL_KIND_INT,
    VAL_KIND_UNDEFINED
} ValueKind;

typedef struct {
    ValueKind kind;
    union {
        int64_t int_val;
    } as;
} Value;


// --- Core Training Data Structures ---

// Represents a single row in one of the training tables.
// This structure encodes a fundamental rule of computation.
typedef struct {
    Value input_a;
    Value input_b;
    OpKind operation;
    Value result;
    ConflictType conflict;
    const char* resolved_module; // e.g., "{2}" or "Undefined Conflict" [cite: 240]
} TrainingRule;

// Represents a table of training rules, which forms the AI's curriculum.
typedef struct {
    TrainingRule* rules;
    size_t count;
    size_t capacity;
} TrainingTable;


// --- Public API for Managing Training Data ---

// Initializes a training table.
void training_table_init(TrainingTable* table);

// Frees the memory used by a training table.
void training_table_free(TrainingTable* table);

// Adds a new rule to the training table.
void training_table_add_rule(TrainingTable* table, TrainingRule rule);

#endif // AI_TRAINER_TYPES_H
