#include <stdio.h>
#include "../../bdi_pipeline.h"

int main(void) {
    printf("=== GC Debug Test ===\n\n");
    
    // Test 1: GC disabled (should work)
    printf("Test 1: GC DISABLED\n");
    PipelineConfig no_gc_config = pipeline_default_config();
    no_gc_config.enable_gc = false;
    no_gc_config.enable_optimization = false;  // Disable optimization
    no_gc_config.verbose = true;
    
    PipelineResult no_gc_result = pipeline_run_with_config("42;", no_gc_config);
    printf("Result: success=%d, value=%.6f\n\n", no_gc_result.success, no_gc_result.result_value);
    
    // Test 2: GC enabled (this is what we're fixing)
    printf("Test 2: GC ENABLED\n");
    PipelineConfig gc_config = pipeline_default_config();
    gc_config.enable_gc = true;
    gc_config.enable_optimization = false;  // Disable optimization
    gc_config.verbose = true;
    
    PipelineResult gc_result = pipeline_run_with_config("42;", gc_config);
    printf("Result: success=%d, value=%.6f\n\n", gc_result.success, gc_result.result_value);
    
    // Test 3: Arithmetic with GC enabled
    printf("Test 3: GC ENABLED - Arithmetic\n");
    PipelineResult gc_arith_result = pipeline_run_with_config("2 + 3 * 4;", gc_config);
    printf("Result: success=%d, value=%.6f\n\n", gc_arith_result.success, gc_arith_result.result_value);
    
    return 0;
}
