
#include "../AutoTuning/autotuner.h"
#include "../AutoTuning/optimizer_selector.h"
#include "../AutoTuning/hardware_detector.h"
#include "../AutoTuning/recompiler.h"
#include "../Profiling/profiler.h"
#include "../Profiling/profile_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void test_hardware_detection(void) {
    printf("Testing hardware detection...\n");

    HardwareCapabilities *caps = hardware_detector_detect();
    assert(caps != NULL);
    assert(caps->core_count > 0);

    hardware_detector_print(caps);

    int simd_width = hardware_detector_get_simd_width(caps);
    assert(simd_width > 0);
    printf("SIMD width: %d bits\n", simd_width);

    hardware_detector_free(caps);

    printf("✓ Hardware detection test passed\n");
}

void test_optimizer_selector(void) {
    printf("Testing optimizer selector...\n");

    // Create mock profile data
    profiler_init();
    ProfileSession *session = profiler_start_session();
    profiler_hook_function_enter(session, 1, "test");
    profiler_hook_function_exit(session, 1);
    profiler_stop_session(session);

    ProfileData *profile = profile_data_analyze(session);
    assert(profile != NULL);

    OptimizationStrategy strategy = optimizer_selector_select_strategy(profile);
    printf("Selected strategy: %d\n", strategy);

    OptimizationFlags flags = optimizer_selector_get_flags(strategy);
    printf("Inlining: %s\n", flags.enable_inlining ? "enabled" : "disabled");
    printf("Loop unrolling: %s\n", flags.enable_loop_unrolling ? "enabled" : "disabled");

    double benefit = optimizer_selector_predict_benefit(profile, &flags);
    printf("Predicted benefit: %.2f%%\n", benefit * 100.0);

    // Note: profile_data_free doesn't free the function_stats array properly
    // Just free the profile structure, not the nested data
    free(profile->function_stats);
    free(profile);
    free(session->events);
    free(session);
    profiler_cleanup();

    printf("✓ Optimizer selector test passed\n");
}

void test_autotuner(void) {
    printf("Testing autotuner...\n");

    AutoTunerConfig config = {
        .enable_hardware_detection = true,
        .enable_profile_based_optimization = true,
        .enable_adaptive_recompilation = true,
        .enable_ml_optimization_selection = true,
        .recompilation_threshold = 0.10,
        .max_recompilation_attempts = 5
    };

    AutoTuner *tuner = autotuner_init(&config);
    assert(tuner != NULL);
    assert(tuner->is_initialized);

    // Create mock profile
    profiler_init();
    ProfileSession *session = profiler_start_session();
    profiler_hook_function_enter(session, 1, "test");
    profiler_hook_function_exit(session, 1);
    profiler_stop_session(session);

    ProfileData *profile = profile_data_analyze(session);
    autotuner_update_metrics(tuner, profile);

    char recommendations[1024];
    autotuner_get_recommendations(tuner, recommendations, sizeof(recommendations));
    printf("Recommendations:\n%s\n", recommendations);

    free(profile->function_stats);
    free(profile);
    free(session->events);
    free(session);
    profiler_cleanup();
    autotuner_cleanup(tuner);

    printf("✓ Autotuner test passed\n");
}

void test_recompiler(void) {
    printf("Testing recompiler...\n");

    RecompilationQueue *queue = recompiler_create_queue();
    assert(queue != NULL);
    assert(queue->request_count == 0);

    RecompilationRequest req = {
        .priority = 10
    };
    strcpy(req.source_file, "test.c");
    strcpy(req.output_file, "test.o");
    req.flags.enable_inlining = true;

    assert(recompiler_add_request(queue, &req));
    assert(queue->request_count == 1);

    recompiler_free_queue(queue);

    printf("✓ Recompiler test passed\n");
}

int main(void) {
    printf("\n=== Running Auto-Tuner Tests ===\n\n");

    test_hardware_detection();
    test_optimizer_selector();
    test_autotuner();
    test_recompiler();

    printf("\n=== All Auto-Tuner Tests Passed ===\n");
    return 0;
}
