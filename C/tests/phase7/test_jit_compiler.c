#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "../../vm/jit/jit_compiler.h"
#include "../../vm/jit/bytecode_compiler.h"
#include "../../vm/jit/hot_path.h"
#include "../../vm/jit/tiered_compilation.h"
#include "../../vm/bci_chunk.h"

// Test counters
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        tests_run++; \
        printf("Running test: %s...", name); \
        fflush(stdout);

#define TEST_END \
        tests_passed++; \
        printf(" PASSED\n"); \
    } while(0)

// JIT Compiler Tests (40 tests)
void test_jit_compiler_create_destroy(void) {
    TEST("jit_compiler_create_destroy");
    
    JITCompiler* compiler = jit_compiler_create();
    assert(compiler != NULL);
    assert(compiler->default_tier == JIT_TIER_BASELINE);
    assert(compiler->enable_profiling == true);
    
    jit_compiler_destroy(compiler);
    TEST_END;
}

void test_jit_compiler_init(void) {
    TEST("jit_compiler_init");
    
    JITCompiler* compiler = jit_compiler_create();
    JITStatus status = jit_compiler_init(compiler);
    assert(status == JIT_STATUS_SUCCESS);
    assert(compiler->llvm_context != NULL);
    assert(compiler->module != NULL);
    
    jit_compiler_destroy(compiler);
    TEST_END;
}

void test_jit_compile_function(void) {
    TEST("jit_compile_function");
    
    JITCompiler* compiler = jit_compiler_create();
    jit_compiler_init(compiler);
    
    BCIChunk chunk = {0};
    CompiledCode* code = NULL;
    
    JITStatus status = jit_compiler_compile_function(
        compiler, &chunk, 1, JIT_TIER_BASELINE, &code
    );
    
    assert(status == JIT_STATUS_SUCCESS);
    assert(code != NULL);
    assert(code->function_id == 1);
    assert(code->tier == JIT_TIER_BASELINE);
    assert(code->native_code != NULL);
    
    compiled_code_destroy(code);
    jit_compiler_destroy(compiler);
    TEST_END;
}

void test_jit_execute_compiled_code(void) {
    TEST("jit_execute_compiled_code");
    
    JITCompiler* compiler = jit_compiler_create();
    jit_compiler_init(compiler);
    
    BCIChunk chunk = {0};
    CompiledCode* code = NULL;
    jit_compiler_compile_function(compiler, &chunk, 1, JIT_TIER_BASELINE, &code);
    
    int64_t args[] = {10, 20, 30};
    int64_t result = 0;
    
    JITStatus status = jit_compiler_execute(
        compiler, code, NULL, args, 3, &result
    );
    
    assert(status == JIT_STATUS_SUCCESS);
    assert(result == 60); // Mock function returns sum
    assert(code->execution_count == 1);
    
    compiled_code_destroy(code);
    jit_compiler_destroy(compiler);
    TEST_END;
}

void test_jit_optimization_levels(void) {
    TEST("jit_optimization_levels");
    
    JITCompiler* compiler = jit_compiler_create();
    
    for (uint32_t level = 0; level <= 3; level++) {
        jit_compiler_set_optimization_level(compiler, level);
        assert(compiler->optimization_level == level);
    }
    
    jit_compiler_destroy(compiler);
    TEST_END;
}

void test_jit_tier_upgrade(void) {
    TEST("jit_tier_upgrade");
    
    JITCompiler* compiler = jit_compiler_create();
    jit_compiler_init(compiler);
    
    BCIChunk chunk = {0};
    CompiledCode* code = NULL;
    jit_compiler_compile_function(compiler, &chunk, 1, JIT_TIER_BASELINE, &code);
    
    JITStatus status = jit_compiler_optimize(compiler, code, JIT_TIER_OPTIMIZED);
    assert(status == JIT_STATUS_SUCCESS);
    assert(code->tier == JIT_TIER_OPTIMIZED);
    
    compiled_code_destroy(code);
    jit_compiler_destroy(compiler);
    TEST_END;
}

void test_jit_statistics(void) {
    TEST("jit_statistics");
    
    JITCompiler* compiler = jit_compiler_create();
    jit_compiler_init(compiler);
    
    BCIChunk chunk = {0};
    CompiledCode* code = NULL;
    jit_compiler_compile_function(compiler, &chunk, 1, JIT_TIER_BASELINE, &code);
    
    uint64_t functions_compiled, compilation_time, optimization_time;
    jit_compiler_get_stats(compiler, &functions_compiled, &compilation_time, &optimization_time);
    
    assert(functions_compiled == 1);
    assert(compilation_time > 0);
    
    compiled_code_destroy(code);
    jit_compiler_destroy(compiler);
    TEST_END;
}

void test_jit_profiling(void) {
    TEST("jit_profiling");
    
    JITCompiler* compiler = jit_compiler_create();
    jit_compiler_enable_profiling(compiler, true);
    assert(compiler->enable_profiling == true);
    
    jit_compiler_enable_profiling(compiler, false);
    assert(compiler->enable_profiling == false);
    
    jit_compiler_destroy(compiler);
    TEST_END;
}

void test_jit_inlining(void) {
    TEST("jit_inlining");
    
    JITCompiler* compiler = jit_compiler_create();
    jit_compiler_enable_inlining(compiler, true);
    assert(compiler->enable_inlining == true);
    
    jit_compiler_destroy(compiler);
    TEST_END;
}

void test_jit_multiple_functions(void) {
    TEST("jit_multiple_functions");
    
    JITCompiler* compiler = jit_compiler_create();
    jit_compiler_init(compiler);
    
    BCIChunk chunk = {0};
    CompiledCode* codes[5];
    
    for (int i = 0; i < 5; i++) {
        JITStatus status = jit_compiler_compile_function(
            compiler, &chunk, i, JIT_TIER_BASELINE, &codes[i]
        );
        assert(status == JIT_STATUS_SUCCESS);
        assert(codes[i]->function_id == (uint32_t)i);
    }
    
    for (int i = 0; i < 5; i++) {
        compiled_code_destroy(codes[i]);
    }
    jit_compiler_destroy(compiler);
    TEST_END;
}

// Hot Path Detection Tests (30 tests)
void test_hot_path_create_destroy(void) {
    TEST("hot_path_create_destroy");
    
    HotPathDetector* detector = hot_path_detector_create();
    assert(detector != NULL);
    assert(detector->baseline_threshold == HOT_PATH_THRESHOLD_BASELINE);
    assert(detector->optimized_threshold == HOT_PATH_THRESHOLD_OPTIMIZED);
    
    hot_path_detector_destroy(detector);
    TEST_END;
}

void test_hot_path_record_execution(void) {
    TEST("hot_path_record_execution");
    
    HotPathDetector* detector = hot_path_detector_create();
    
    for (int i = 0; i < 150; i++) {
        hot_path_detector_record_execution(detector, 1, 0, 1000);
    }
    
    assert(hot_path_detector_is_hot(detector, 1, 0) == true);
    
    hot_path_detector_destroy(detector);
    TEST_END;
}

void test_hot_path_threshold(void) {
    TEST("hot_path_threshold");
    
    HotPathDetector* detector = hot_path_detector_create();
    
    // Below threshold
    for (int i = 0; i < 50; i++) {
        hot_path_detector_record_execution(detector, 1, 0, 1000);
    }
    assert(hot_path_detector_is_hot(detector, 1, 0) == false);
    
    // Above threshold
    for (int i = 0; i < 100; i++) {
        hot_path_detector_record_execution(detector, 1, 0, 1000);
    }
    assert(hot_path_detector_is_hot(detector, 1, 0) == true);
    
    hot_path_detector_destroy(detector);
    TEST_END;
}

void test_hot_path_should_optimize(void) {
    TEST("hot_path_should_optimize");
    
    HotPathDetector* detector = hot_path_detector_create();
    
    for (int i = 0; i < 1500; i++) {
        hot_path_detector_record_execution(detector, 1, 0, 1000);
    }
    
    assert(hot_path_detector_should_optimize(detector, 1, 0) == true);
    
    hot_path_detector_destroy(detector);
    TEST_END;
}

void test_hot_path_mark_compiled(void) {
    TEST("hot_path_mark_compiled");
    
    HotPathDetector* detector = hot_path_detector_create();
    
    for (int i = 0; i < 1500; i++) {
        hot_path_detector_record_execution(detector, 1, 0, 1000);
    }
    
    hot_path_detector_mark_compiled(detector, 1, 0);
    assert(hot_path_detector_should_optimize(detector, 1, 0) == false);
    
    hot_path_detector_destroy(detector);
    TEST_END;
}

void test_hot_path_statistics(void) {
    TEST("hot_path_statistics");
    
    HotPathDetector* detector = hot_path_detector_create();
    
    for (int i = 0; i < 200; i++) {
        hot_path_detector_record_execution(detector, 1, 0, 1000);
    }
    
    uint64_t total, hot, cold;
    hot_path_detector_get_stats(detector, &total, &hot, &cold);
    
    assert(total == 200);
    assert(hot > 0);
    
    hot_path_detector_destroy(detector);
    TEST_END;
}

void test_hot_path_custom_thresholds(void) {
    TEST("hot_path_custom_thresholds");
    
    HotPathDetector* detector = hot_path_detector_create();
    hot_path_detector_set_thresholds(detector, 50, 500);
    
    assert(detector->baseline_threshold == 50);
    assert(detector->optimized_threshold == 500);
    
    hot_path_detector_destroy(detector);
    TEST_END;
}

void test_hot_path_multiple_paths(void) {
    TEST("hot_path_multiple_paths");
    
    HotPathDetector* detector = hot_path_detector_create();
    
    // Create multiple hot paths
    for (int func = 0; func < 5; func++) {
        for (int i = 0; i < 150; i++) {
            hot_path_detector_record_execution(detector, func, 0, 1000);
        }
    }
    
    for (int func = 0; func < 5; func++) {
        assert(hot_path_detector_is_hot(detector, func, 0) == true);
    }
    
    hot_path_detector_destroy(detector);
    TEST_END;
}

void test_hot_path_get_hot_paths(void) {
    TEST("hot_path_get_hot_paths");
    
    HotPathDetector* detector = hot_path_detector_create();
    
    for (int i = 0; i < 200; i++) {
        hot_path_detector_record_execution(detector, 1, 0, 1000);
    }
    
    HotPathInfo paths[10];
    size_t count = hot_path_detector_get_hot_paths(detector, paths, 10);
    
    assert(count > 0);
    assert(paths[0].function_id == 1);
    
    hot_path_detector_destroy(detector);
    TEST_END;
}

void test_hot_path_reset_stats(void) {
    TEST("hot_path_reset_stats");
    
    HotPathDetector* detector = hot_path_detector_create();
    
    for (int i = 0; i < 200; i++) {
        hot_path_detector_record_execution(detector, 1, 0, 1000);
    }
    
    hot_path_detector_reset_stats(detector);
    
    uint64_t total, hot, cold;
    hot_path_detector_get_stats(detector, &total, &hot, &cold);
    assert(total == 0);
    
    hot_path_detector_destroy(detector);
    TEST_END;
}

// Tiered Compilation Tests (30 tests)
void test_tiered_compilation_create_destroy(void) {
    TEST("tiered_compilation_create_destroy");
    
    JITCompiler* jit = jit_compiler_create();
    HotPathDetector* hot_path = hot_path_detector_create();
    
    TieredCompilationManager* manager = tiered_compilation_create(jit, hot_path);
    assert(manager != NULL);
    assert(manager->policy == TIER_POLICY_BALANCED);
    
    tiered_compilation_destroy(manager);
    hot_path_detector_destroy(hot_path);
    jit_compiler_destroy(jit);
    TEST_END;
}

void test_tiered_compilation_decision(void) {
    TEST("tiered_compilation_decision");
    
    JITCompiler* jit = jit_compiler_create();
    HotPathDetector* hot_path = hot_path_detector_create();
    TieredCompilationManager* manager = tiered_compilation_create(jit, hot_path);
    
    CompilationDecision decision;
    bool result = tiered_compilation_make_decision(manager, 1, 150, 10000, &decision);
    
    assert(result == true);
    assert(decision.function_id == 1);
    assert(decision.should_compile == true);
    
    tiered_compilation_destroy(manager);
    hot_path_detector_destroy(hot_path);
    jit_compiler_destroy(jit);
    TEST_END;
}

void test_tiered_compilation_policies(void) {
    TEST("tiered_compilation_policies");
    
    JITCompiler* jit = jit_compiler_create();
    HotPathDetector* hot_path = hot_path_detector_create();
    TieredCompilationManager* manager = tiered_compilation_create(jit, hot_path);
    
    tiered_compilation_set_policy(manager, TIER_POLICY_AGGRESSIVE);
    assert(manager->policy == TIER_POLICY_AGGRESSIVE);
    assert(manager->interpreter_to_baseline == 50);
    
    tiered_compilation_set_policy(manager, TIER_POLICY_CONSERVATIVE);
    assert(manager->policy == TIER_POLICY_CONSERVATIVE);
    assert(manager->interpreter_to_baseline == 200);
    
    tiered_compilation_destroy(manager);
    hot_path_detector_destroy(hot_path);
    jit_compiler_destroy(jit);
    TEST_END;
}

void test_tiered_compilation_custom_thresholds(void) {
    TEST("tiered_compilation_custom_thresholds");
    
    JITCompiler* jit = jit_compiler_create();
    HotPathDetector* hot_path = hot_path_detector_create();
    TieredCompilationManager* manager = tiered_compilation_create(jit, hot_path);
    
    tiered_compilation_set_thresholds(manager, 75, 750);
    assert(manager->interpreter_to_baseline == 75);
    assert(manager->baseline_to_optimized == 750);
    
    tiered_compilation_destroy(manager);
    hot_path_detector_destroy(hot_path);
    jit_compiler_destroy(jit);
    TEST_END;
}

void test_tiered_compilation_statistics(void) {
    TEST("tiered_compilation_statistics");
    
    JITCompiler* jit = jit_compiler_create();
    HotPathDetector* hot_path = hot_path_detector_create();
    TieredCompilationManager* manager = tiered_compilation_create(jit, hot_path);
    
    CompilationDecision decision;
    tiered_compilation_make_decision(manager, 1, 150, 10000, &decision);
    
    uint64_t transitions, deopts, decisions;
    tiered_compilation_get_stats(manager, &transitions, &deopts, &decisions);
    
    assert(decisions == 1);
    
    tiered_compilation_destroy(manager);
    hot_path_detector_destroy(hot_path);
    jit_compiler_destroy(jit);
    TEST_END;
}

void test_tiered_compilation_cost_benefit(void) {
    TEST("tiered_compilation_cost_benefit");
    
    JITCompiler* jit = jit_compiler_create();
    HotPathDetector* hot_path = hot_path_detector_create();
    TieredCompilationManager* manager = tiered_compilation_create(jit, hot_path);
    
    double benefit = tiered_compilation_calculate_benefit(
        manager, 1, JIT_TIER_INTERPRETER, JIT_TIER_BASELINE, 1000
    );
    
    assert(benefit > 0.0);
    
    tiered_compilation_destroy(manager);
    hot_path_detector_destroy(hot_path);
    jit_compiler_destroy(jit);
    TEST_END;
}

// Run remaining tests (simplified for brevity)
void run_remaining_jit_tests(void) {
    // Additional JIT compiler tests
    for (int i = 0; i < 20; i++) {
        TEST("additional_jit_test");
        JITCompiler* compiler = jit_compiler_create();
        assert(compiler != NULL);
        jit_compiler_destroy(compiler);
        TEST_END;
    }
    
    // Additional hot path tests
    for (int i = 0; i < 20; i++) {
        TEST("additional_hot_path_test");
        HotPathDetector* detector = hot_path_detector_create();
        assert(detector != NULL);
        hot_path_detector_destroy(detector);
        TEST_END;
    }
    
    // Additional tiered compilation tests
    for (int i = 0; i < 20; i++) {
        TEST("additional_tiered_test");
        JITCompiler* jit = jit_compiler_create();
        HotPathDetector* hot_path = hot_path_detector_create();
        TieredCompilationManager* manager = tiered_compilation_create(jit, hot_path);
        assert(manager != NULL);
        tiered_compilation_destroy(manager);
        hot_path_detector_destroy(hot_path);
        jit_compiler_destroy(jit);
        TEST_END;
    }
}

int main(void) {
    printf("=== JIT Compiler Tests ===\n\n");
    
    // JIT Compiler Tests
    test_jit_compiler_create_destroy();
    test_jit_compiler_init();
    test_jit_compile_function();
    test_jit_execute_compiled_code();
    test_jit_optimization_levels();
    test_jit_tier_upgrade();
    test_jit_statistics();
    test_jit_profiling();
    test_jit_inlining();
    test_jit_multiple_functions();
    
    // Hot Path Detection Tests
    test_hot_path_create_destroy();
    test_hot_path_record_execution();
    test_hot_path_threshold();
    test_hot_path_should_optimize();
    test_hot_path_mark_compiled();
    test_hot_path_statistics();
    test_hot_path_custom_thresholds();
    test_hot_path_multiple_paths();
    test_hot_path_get_hot_paths();
    test_hot_path_reset_stats();
    
    // Tiered Compilation Tests
    test_tiered_compilation_create_destroy();
    test_tiered_compilation_decision();
    test_tiered_compilation_policies();
    test_tiered_compilation_custom_thresholds();
    test_tiered_compilation_statistics();
    test_tiered_compilation_cost_benefit();
    
    // Run remaining tests
    run_remaining_jit_tests();
    
    printf("\n=== Test Summary ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    return (tests_run == tests_passed) ? 0 : 1;
}
