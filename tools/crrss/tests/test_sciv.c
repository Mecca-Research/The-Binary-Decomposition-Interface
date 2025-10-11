
/**
 * @file test_sciv.c
 * @brief Tests for Self-Check Internal Validator
 */

#include "../sciv/sciv.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

void test_sciv_initialization() {
    printf("Testing SCIV initialization...\n");
    
    sciv_config_t config = {
        .enable_strict_mode = false,
        .enable_style_checks = true,
        .enable_performance_checks = true,
        .max_function_complexity = 20,
        .max_function_lines = 200,
        .max_cyclomatic_complexity = 15,
        .coding_standard = "kernel"
    };
    
    sciv_context_t* ctx = sciv_initialize(&config);
    assert(ctx != NULL);
    
    sciv_shutdown(ctx);
    printf("  ✓ SCIV initialization test passed\n");
}

void test_sciv_validate_snippet() {
    printf("Testing SCIV snippet validation...\n");
    
    sciv_config_t config = {
        .enable_strict_mode = false,
        .enable_style_checks = true,
        .enable_performance_checks = true,
        .max_function_complexity = 20,
        .max_function_lines = 200,
        .max_cyclomatic_complexity = 15,
        .coding_standard = "kernel"
    };
    
    sciv_context_t* ctx = sciv_initialize(&config);
    assert(ctx != NULL);
    
    const char* code = "strcpy(dest, src);  // Unsafe";
    validation_issue_t issues[10];
    uint32_t num_issues = 0;
    
    crrss_status_t status = sciv_validate_snippet(
        ctx, code, strlen(code), issues, 10, &num_issues
    );
    
    assert(status == CRRSS_SUCCESS);
    printf("  Found %u validation issues\n", num_issues);
    
    // Cleanup
    for (uint32_t i = 0; i < num_issues; i++) {
        free((void*)issues[i].file_path);
        free((void*)issues[i].message);
        free((void*)issues[i].suggestion);
    }
    
    sciv_shutdown(ctx);
    printf("  ✓ SCIV snippet validation test passed\n");
}

void test_sciv_rule_configuration() {
    printf("Testing SCIV rule configuration...\n");
    
    sciv_config_t config = {
        .enable_strict_mode = false,
        .enable_style_checks = true,
        .enable_performance_checks = true,
        .max_function_complexity = 20,
        .max_function_lines = 200,
        .max_cyclomatic_complexity = 15,
        .coding_standard = "kernel"
    };
    
    sciv_context_t* ctx = sciv_initialize(&config);
    assert(ctx != NULL);
    
    // Disable a rule
    crrss_status_t status = sciv_configure_rule(ctx, RULE_MEMORY_SAFETY, false);
    assert(status == CRRSS_SUCCESS);
    
    // Enable it again
    status = sciv_configure_rule(ctx, RULE_MEMORY_SAFETY, true);
    assert(status == CRRSS_SUCCESS);
    
    sciv_shutdown(ctx);
    printf("  ✓ SCIV rule configuration test passed\n");
}

void test_sciv_statistics() {
    printf("Testing SCIV statistics...\n");
    
    sciv_config_t config = {
        .enable_strict_mode = false,
        .enable_style_checks = true,
        .enable_performance_checks = true,
        .max_function_complexity = 20,
        .max_function_lines = 200,
        .max_cyclomatic_complexity = 15,
        .coding_standard = "kernel"
    };
    
    sciv_context_t* ctx = sciv_initialize(&config);
    assert(ctx != NULL);
    
    uint32_t total_validations = 0;
    uint32_t total_issues = 0;
    double avg_compliance = 0.0;
    
    crrss_status_t status = sciv_get_statistics(
        ctx, &total_validations, &total_issues, &avg_compliance
    );
    assert(status == CRRSS_SUCCESS);
    
    printf("  Total validations: %u\n", total_validations);
    printf("  Total issues: %u\n", total_issues);
    
    sciv_shutdown(ctx);
    printf("  ✓ SCIV statistics test passed\n");
}

int main(void) {
    printf("=== Running SCIV Tests ===\n\n");
    
    test_sciv_initialization();
    test_sciv_validate_snippet();
    test_sciv_rule_configuration();
    test_sciv_statistics();
    
    printf("\n=== All SCIV Tests Passed ===\n");
    return 0;
}
