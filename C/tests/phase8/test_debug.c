#include <stdio.h>
#include "../../bdi_pipeline.h"

int main(void) {
    PipelineConfig config = pipeline_default_config();
    config.verbose = true;
    
    printf("Testing: 42;\n");
    PipelineResult result = pipeline_run_with_config("42;", config);
    
    printf("\nResult:\n");
    printf("  Success: %d\n", result.success);
    printf("  Value: %.2f\n", result.result_value);
    printf("  Error: %s\n", result.error_message ? result.error_message : "none");
    printf("  Bytecode size: %zu\n", result.bytecode_size);
    
    return 0;
}
