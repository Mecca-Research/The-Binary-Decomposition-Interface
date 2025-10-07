
#ifndef BDI_AUTOTUNER_H
#define BDI_AUTOTUNER_H

#include "../Profiling/profile_data.h"
#include "../ModelFormat/bdi_model.h"
#include <stdbool.h>

// Auto-tuning configuration
typedef struct {
    bool enable_hardware_detection;
    bool enable_profile_based_optimization;
    bool enable_adaptive_recompilation;
    bool enable_ml_optimization_selection;
    double recompilation_threshold;  // Performance improvement threshold
    int max_recompilation_attempts;
} AutoTunerConfig;

// Auto-tuner state
typedef struct {
    AutoTunerConfig config;
    ProfileData *baseline_profile;
    ProfileData *current_profile;
    int recompilation_count;
    double best_performance;
    bool is_initialized;
} AutoTuner;

// Initialize auto-tuner
AutoTuner* autotuner_init(const AutoTunerConfig *config);

// Cleanup auto-tuner
void autotuner_cleanup(AutoTuner *tuner);

// Analyze profile and determine if recompilation is needed
bool autotuner_should_recompile(AutoTuner *tuner, const ProfileData *profile);

// Trigger adaptive recompilation
bool autotuner_trigger_recompilation(AutoTuner *tuner, const char *source_file);

// Update performance metrics
void autotuner_update_metrics(AutoTuner *tuner, const ProfileData *profile);

// Get optimization recommendations
void autotuner_get_recommendations(const AutoTuner *tuner, char *buffer, size_t buffer_size);

#endif // BDI_AUTOTUNER_H
