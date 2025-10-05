
// ===================================================================
// Phase 5.4: HAM Entropy-Based Scoring
// DESC: Entropy computation for intelligent memory management
// ===================================================================
/**
 * @file ham_entropy.h
 * @brief Ham Entropy API
 * @details This file provides the ham entropy functionality for the BDI system.
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef AEON_HAM_ENTROPY_H
#define AEON_HAM_ENTROPY_H

#include "../../c23_compat.h"
#include "../../ham/ham.h"
#include <stdint.h>
#include <stddef.h>

// --- Entropy Computation ---
[[nodiscard]] float ham_compute_entropy(const void* data, size_t size);
[[nodiscard]] int ham_update_entropy_score(HamRegion* region);

// --- Access Pattern Analysis ---
typedef struct {
    uint64_t* access_history;
    size_t history_size;
    size_t history_capacity;
    float entropy_score;
} AccessPattern;

[[nodiscard]] AccessPattern* access_pattern_create(size_t capacity);
void access_pattern_free(AccessPattern* pattern);
[[nodiscard]] int access_pattern_record(AccessPattern* pattern, uint64_t cycle);
[[nodiscard]] float access_pattern_compute_entropy(const AccessPattern* pattern);

#endif // AEON_HAM_ENTROPY_H
