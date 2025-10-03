// ===================================================================
// DESC: The main driver for the AI Training Engine. This file
//       initializes, runs, and cleans up the trainer, demonstrating
//       the complete Structured Intelligence Formation process.
// ===================================================================

#include "c23_compat.h"
#include "ai_trainer.h"
#include <stdio.h>

int main(int argc, const char* argv[]) {
    printf("============================================\n");
    printf("  Initializing Sentient Compiler AI Trainer \n");
    printf("============================================\n\n");

    // 1. Initialize the AI Trainer.
    // This also loads the foundational curriculum from the trainer's source.
    AITrainer trainer;
    ai_trainer_init(&trainer);

    // 2. Run the main training session.
    // This simulates the AI learning from the structured rules.
    ai_trainer_run(&trainer);

    // 3. Free all resources.
    // This cleans up the curriculum and any other allocated memory.
    ai_trainer_free(&trainer);

    printf("\n============================================\n");
    printf("  AI Trainer Shutdown Complete.             \n");
    printf("============================================\n");

    return 0;
}
