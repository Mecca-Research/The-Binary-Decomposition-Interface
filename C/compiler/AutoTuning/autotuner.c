
#include "autotuner.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

AutoTuner* autotuner_init(const AutoTunerConfig *config) {
    AutoTuner *tuner = calloc(1, sizeof(AutoTuner));
    if (!tuner) {
        return NULL;
    }

    if (config) {
        tuner->config = *config;
    } else {
        // Default configuration
        tuner->config.enable_hardware_detection = true;
        tuner->config.enable_profile_based_optimization = true;
        tuner->config.enable_adaptive_recompilation = true;
        tuner->config.enable_ml_optimization_selection = true;
        tuner->config.recompilation_threshold = 0.10;  // 10% improvement
        tuner->config.max_recompilation_attempts = 5;
    }

    tuner->is_initialized = true;
    return tuner;
}

void autotuner_cleanup(AutoTuner *tuner) {
    if (!tuner) return;
    
    if (tuner->baseline_profile) {
        profile_data_free(tuner->baseline_profile);
    }
    if (tuner->current_profile) {
        profile_data_free(tuner->current_profile);
    }
    
    free(tuner);
}

bool autotuner_should_recompile(AutoTuner *tuner, const ProfileData *profile) {
    if (!tuner || !profile || !tuner->config.enable_adaptive_recompilation) {
        return false;
    }

    // Check if we've exceeded max attempts
    if (tuner->recompilation_count >= tuner->config.max_recompilation_attempts) {
        return false;
    }

    // If no baseline, set it and don't recompile yet
    if (!tuner->baseline_profile) {
        return false;
    }

    // Calculate performance improvement potential
    double current_time = (double)profile->total_execution_time_ns;
    double baseline_time = (double)tuner->baseline_profile->total_execution_time_ns;

    if (baseline_time == 0) {
        return false;
    }

    double improvement = (baseline_time - current_time) / baseline_time;

    // If performance degraded significantly, consider recompilation
    if (improvement < -tuner->config.recompilation_threshold) {
        return true;
    }

    // Check for specific issues that warrant recompilation
    if (profile->cache_stats.hit_rate < 0.80) {
        return true;
    }

    if (profile->memory_stats.peak_memory_usage > 
        tuner->baseline_profile->memory_stats.peak_memory_usage * 1.5) {
        return true;
    }

    return false;
}

bool autotuner_trigger_recompilation(AutoTuner *tuner, const char *source_file) {
    if (!tuner || !source_file) {
        return false;
    }

    tuner->recompilation_count++;

    // TODO: Integrate with compiler to trigger actual recompilation
    // For now, just log the intent
    printf("Auto-tuner: Triggering recompilation of %s (attempt %d/%d)\n",
           source_file, tuner->recompilation_count, 
           tuner->config.max_recompilation_attempts);

    return true;
}

void autotuner_update_metrics(AutoTuner *tuner, const ProfileData *profile) {
    if (!tuner || !profile) {
        return;
    }

    // Set baseline if not set
    if (!tuner->baseline_profile) {
        // Just store the execution time, not the full profile
        tuner->best_performance = (double)profile->total_execution_time_ns;
        return;
    }

    // Update best performance
    double current_time = (double)profile->total_execution_time_ns;
    if (current_time < tuner->best_performance) {
        tuner->best_performance = current_time;
    }
}

void autotuner_get_recommendations(const AutoTuner *tuner, char *buffer, size_t buffer_size) {
    if (!tuner || !buffer || buffer_size == 0) {
        return;
    }

    buffer[0] = '\0';
    
    char temp[512];
    
    snprintf(temp, sizeof(temp), "Auto-tuner Recommendations:\n");
    strncat(buffer, temp, buffer_size - strlen(buffer) - 1);

    if (tuner->config.enable_hardware_detection) {
        snprintf(temp, sizeof(temp), "- Hardware detection: ENABLED\n");
        strncat(buffer, temp, buffer_size - strlen(buffer) - 1);
    }

    if (tuner->config.enable_ml_optimization_selection) {
        snprintf(temp, sizeof(temp), "- ML optimization selection: ENABLED\n");
        strncat(buffer, temp, buffer_size - strlen(buffer) - 1);
    }

    snprintf(temp, sizeof(temp), "- Recompilation attempts: %d/%d\n",
             tuner->recompilation_count, tuner->config.max_recompilation_attempts);
    strncat(buffer, temp, buffer_size - strlen(buffer) - 1);

    if (tuner->best_performance > 0) {
        snprintf(temp, sizeof(temp), "- Best performance: %.3f ms\n", 
                tuner->best_performance / 1000000.0);
        strncat(buffer, temp, buffer_size - strlen(buffer) - 1);
    }
}
