
#include "recompiler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define INITIAL_QUEUE_CAPACITY 10

RecompilationQueue* recompiler_create_queue(void) {
    RecompilationQueue *queue = calloc(1, sizeof(RecompilationQueue));
    if (!queue) {
        return NULL;
    }

    queue->requests = calloc(INITIAL_QUEUE_CAPACITY, sizeof(RecompilationRequest));
    if (!queue->requests) {
        free(queue);
        return NULL;
    }

    queue->request_capacity = INITIAL_QUEUE_CAPACITY;
    queue->request_count = 0;

    return queue;
}

void recompiler_free_queue(RecompilationQueue *queue) {
    if (!queue) return;
    
    for (size_t i = 0; i < queue->request_count; i++) {
        if (queue->requests[i].profile) {
            profile_data_free(queue->requests[i].profile);
        }
    }
    
    free(queue->requests);
    free(queue);
}

bool recompiler_add_request(RecompilationQueue *queue, const RecompilationRequest *request) {
    if (!queue || !request) {
        return false;
    }

    // Expand capacity if needed
    if (queue->request_count >= queue->request_capacity) {
        size_t new_capacity = queue->request_capacity * 2;
        RecompilationRequest *new_requests = realloc(queue->requests,
                                                     new_capacity * sizeof(RecompilationRequest));
        if (!new_requests) {
            return false;
        }
        queue->requests = new_requests;
        queue->request_capacity = new_capacity;
    }

    queue->requests[queue->request_count++] = *request;
    return true;
}

static int compare_requests(const void *a, const void *b) {
    const RecompilationRequest *ra = a;
    const RecompilationRequest *rb = b;
    return rb->priority - ra->priority;  // Higher priority first
}

bool recompiler_process_queue(RecompilationQueue *queue) {
    if (!queue || queue->request_count == 0) {
        return true;
    }

    // Sort by priority
    qsort(queue->requests, queue->request_count, sizeof(RecompilationRequest), compare_requests);

    // Process each request
    for (size_t i = 0; i < queue->request_count; i++) {
        RecompilationRequest *req = &queue->requests[i];
        
        printf("Processing recompilation request %zu/%zu: %s\n",
               i + 1, queue->request_count, req->source_file);

        if (!recompiler_recompile_file(req->source_file, req->output_file, &req->flags)) {
            fprintf(stderr, "Failed to recompile %s\n", req->source_file);
            return false;
        }
    }

    // Clear queue
    queue->request_count = 0;

    return true;
}

bool recompiler_recompile_file(const char *source_file, const char *output_file,
                               const OptimizationFlags *flags) {
    if (!source_file || !output_file || !flags) {
        return false;
    }

    // Build compiler command
    char command[1024];
    snprintf(command, sizeof(command), "gcc -std=gnu2x -Wall -Wextra");

    // Add optimization flags
    if (flags->enable_inlining) {
        strncat(command, " -finline-functions", sizeof(command) - strlen(command) - 1);
        char threshold[64];
        snprintf(threshold, sizeof(threshold), " --param inline-unit-growth=%d", 
                flags->inline_threshold);
        strncat(command, threshold, sizeof(command) - strlen(command) - 1);
    }

    if (flags->enable_loop_unrolling) {
        strncat(command, " -funroll-loops", sizeof(command) - strlen(command) - 1);
        char factor[64];
        snprintf(factor, sizeof(factor), " --param max-unroll-times=%d", flags->unroll_factor);
        strncat(command, factor, sizeof(command) - strlen(command) - 1);
    }

    if (flags->enable_vectorization) {
        strncat(command, " -ftree-vectorize", sizeof(command) - strlen(command) - 1);
    }

    if (flags->enable_constant_folding) {
        strncat(command, " -O2", sizeof(command) - strlen(command) - 1);
    }

    if (flags->enable_dead_code_elimination) {
        strncat(command, " -fdce", sizeof(command) - strlen(command) - 1);
    }

    if (flags->enable_instruction_scheduling) {
        strncat(command, " -fschedule-insns", sizeof(command) - strlen(command) - 1);
    }

    // Add source and output files
    char files[512];
    snprintf(files, sizeof(files), " -c %s -o %s", source_file, output_file);
    strncat(command, files, sizeof(command) - strlen(command) - 1);

    // Execute compilation
    printf("Executing: %s\n", command);
    int result = system(command);

    return result == 0;
}
