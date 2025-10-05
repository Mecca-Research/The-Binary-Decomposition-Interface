#include <stdio.h>
#include "../../bdi_pipeline.h"

int main(void) {
    printf("Testing with GC DISABLED:\n\n");
    
    PipelineConfig config = pipeline_default_config();
    config.enable_gc = false;  // Disable GC to use basic VM
    config.enable_optimization = false;  // Also disable optimization
    config.verbose = true;
    
    PipelineResult result = pipeline_run_with_config("42;", config);
    
    printf("\n=== Result ===\n");
    printf("Success: %d\n", result.success);
    printf("Result value: %.6f\n", result.result_value);
    
    return 0;
}
