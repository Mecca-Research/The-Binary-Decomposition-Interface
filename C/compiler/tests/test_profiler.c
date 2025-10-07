
#include "../Profiling/profiler.h"
#include "../Profiling/profile_data.h"
#include "../Profiling/profile_serializer.h"
#include "../Profiling/profile_analyzer.h"
#include <stdio.h>
#include <assert.h>

void test_profiler_basic(void) {
    printf("Testing basic profiler functionality...\n");

    assert(profiler_init());

    ProfileSession *session = profiler_start_session();
    assert(session != NULL);
    assert(session->is_active);

    // Simulate some profiling events
    profiler_hook_function_enter(session, 1, "test_function");
    profiler_hook_memory_alloc(session, 0x1000, 1024);
    profiler_hook_cache_access(session, 0x2000, true);
    profiler_hook_branch(session, 0x3000, true);
    profiler_hook_function_exit(session, 1);

    profiler_stop_session(session);
    assert(!session->is_active);
    assert(session->event_count == 5);

    free(session->events);
    free(session);
    profiler_cleanup();

    printf("✓ Basic profiler test passed\n");
}

void test_profile_analysis(void) {
    printf("Testing profile analysis...\n");

    profiler_init();
    ProfileSession *session = profiler_start_session();

    // Simulate function calls
    for (int i = 0; i < 10; i++) {
        profiler_hook_function_enter(session, 1, "hot_function");
        profiler_hook_function_exit(session, 1);
    }

    profiler_stop_session(session);

    ProfileData *data = profile_data_analyze(session);
    assert(data != NULL);
    assert(data->function_count > 0);

    profile_data_print_summary(data);

    profile_data_free(data);
    free(session->events);
    free(session);
    profiler_cleanup();

    printf("✓ Profile analysis test passed\n");
}

void test_profile_serialization(void) {
    printf("Testing profile serialization...\n");

    profiler_init();
    ProfileSession *session = profiler_start_session();

    profiler_hook_function_enter(session, 1, "test_func");
    profiler_hook_function_exit(session, 1);

    profiler_stop_session(session);

    ProfileData *data = profile_data_analyze(session);
    assert(data != NULL);

    // Save and load
    assert(profile_serializer_save(data, "/tmp/test_profile.bdi-profile"));
    ProfileData *loaded = profile_serializer_load("/tmp/test_profile.bdi-profile");
    assert(loaded != NULL);
    assert(loaded->function_count == data->function_count);

    profile_data_free(data);
    profile_data_free(loaded);
    free(session->events);
    free(session);
    profiler_cleanup();

    printf("✓ Profile serialization test passed\n");
}

void test_profile_analyzer(void) {
    printf("Testing profile analyzer...\n");

    profiler_init();
    ProfileSession *session = profiler_start_session();

    // Create a profile with bottlenecks
    for (int i = 0; i < 1000; i++) {
        profiler_hook_function_enter(session, 1, "bottleneck_func");
        profiler_hook_cache_access(session, 0x1000, false);  // Cache miss
        profiler_hook_function_exit(session, 1);
    }

    profiler_stop_session(session);

    ProfileData *data = profile_data_analyze(session);
    assert(data != NULL);

    OptimizationReport *report = profile_analyzer_generate_suggestions(data);
    assert(report != NULL);
    assert(report->suggestion_count > 0);

    profile_analyzer_print_report(report);

    profile_analyzer_free_report(report);
    profile_data_free(data);
    free(session->events);
    free(session);
    profiler_cleanup();

    printf("✓ Profile analyzer test passed\n");
}

int main(void) {
    printf("\n=== Running Profiler Tests ===\n\n");

    test_profiler_basic();
    test_profile_analysis();
    test_profile_serialization();
    test_profile_analyzer();

    printf("\n=== All Profiler Tests Passed ===\n");
    return 0;
}
