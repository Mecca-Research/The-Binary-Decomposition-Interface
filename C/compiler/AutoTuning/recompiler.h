
#ifndef BDI_RECOMPILER_H
#define BDI_RECOMPILER_H

#include "optimizer_selector.h"
#include "../Profiling/profile_data.h"
#include <stdbool.h>

// Recompilation request
typedef struct {
    char source_file[256];
    char output_file[256];
    OptimizationFlags flags;
    ProfileData *profile;
    int priority;
} RecompilationRequest;

// Recompilation queue
typedef struct {
    RecompilationRequest *requests;
    size_t request_count;
    size_t request_capacity;
} RecompilationQueue;

// Create recompilation queue
RecompilationQueue* recompiler_create_queue(void);

// Free recompilation queue
void recompiler_free_queue(RecompilationQueue *queue);

// Add recompilation request
bool recompiler_add_request(RecompilationQueue *queue, const RecompilationRequest *request);

// Process recompilation queue
bool recompiler_process_queue(RecompilationQueue *queue);

// Recompile single file
bool recompiler_recompile_file(const char *source_file, const char *output_file,
                               const OptimizationFlags *flags);

#endif // BDI_RECOMPILER_H
