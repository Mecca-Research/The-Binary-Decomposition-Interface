#include <stdio.h>
#include "../../bdi_pipeline.h"

int main(void) {
    printf("Testing simple literal: 42;\n");
    
    PipelineConfig config = pipeline_default_config();
    config.verbose = true;  // Enable verbose output
    
    PipelineResult result = pipeline_run_with_config("42;", config);
    
    printf("\n=== Result ===\n");
    printf("Success: %d\n", result.success);
    printf("Result value: %.6f\n", result.result_value);
    printf("Error message: %s\n", result.error_message ? result.error_message : "none");
    printf("Error line: %d\n", result.error_line);
    
    return 0;
}
