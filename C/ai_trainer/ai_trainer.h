// ===================================================================
// DESC: Defines the main AI Training Engine, which orchestrates the
//       structured learning process.
// ===================================================================
/**
 * @file ai_trainer.h
 * @brief Ai Trainer API
 * @details This file provides the ai trainer functionality for the BDI system.
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef AI_TRAINER_H
#define AI_TRAINER_H

#include "c23_compat.h"
#include "ai_trainer_types.h"

// --- AI Trainer Structure ---
// Holds the state of the training process, including the curriculum.
typedef struct {
    TrainingTable curriculum;
} AITrainer;


// --- Public API for the Training Engine ---

// Initializes the AI trainer and loads the foundational curriculum.
void ai_trainer_init(AITrainer* trainer);

// Frees all resources used by the trainer, including the curriculum.
void ai_trainer_free(AITrainer* trainer);

// Runs the main training loop, processing the rules in the curriculum.
void ai_trainer_run(AITrainer* trainer);



// Compile-time invariants
static_assert(sizeof(void*) >= 4, "AI Trainer requires at least 32-bit pointers");
static_assert(sizeof(float) == 4, "float must be 4 bytes");
static_assert(sizeof(double) == 8, "double must be 8 bytes");

#endif // AI_TRAINER_H
