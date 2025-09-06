// ===================================================================
// DESC: Defines the main AI Training Engine, which orchestrates the
//       structured learning process.
// ===================================================================
#ifndef AI_TRAINER_H
#define AI_TRAINER_H

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


#endif // AI_TRAINER_H
