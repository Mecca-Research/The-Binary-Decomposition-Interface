
#ifndef BDI_REGISTER_PREDICTOR_H
#define BDI_REGISTER_PREDICTOR_H

#include "../Profiling/profile_data.h"
#include <stdbool.h>
#include <stddef.h>

// Register allocation prediction
typedef struct {
    int register_id;
    char variable_name[64];
    double spill_probability;
    int priority;
} RegisterAllocation;

// Initialize register predictor
bool register_predictor_init(void);

// Cleanup register predictor
void register_predictor_cleanup(void);

// Predict optimal register allocation
RegisterAllocation* register_predictor_predict(const char *function_code, 
                                               const ProfileData *profile,
                                               size_t *allocation_count);

// Train predictor with profile data
bool register_predictor_train(const ProfileData *profile);

// Free register allocation
void register_predictor_free_allocation(RegisterAllocation *allocation);

#endif // BDI_REGISTER_PREDICTOR_H
