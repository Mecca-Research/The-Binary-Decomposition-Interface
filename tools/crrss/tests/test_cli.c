/**
 * @file test_cli.c
 * @brief Unit tests for CRRSS CLI tool
 * 
 * Tests all CLI functionality including:
 * - Command parsing
 * - Profile management
 * - Configuration management
 * - Consultation module
 * - Validation
 * - Bug pattern lookup
 */

#include "../cli/crrss_cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

// Test counters
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Helper macros
#define TEST_START(name) \
    do { \
        printf("  Running: %s...", name); \
        fflush(stdout); \
        tests_run++; \
    } while(0)

#define TEST_PASS() \
    do { \
        printf(" PASS\n"); \
        tests_passed++; \
    } while(0)

#define TEST_FAIL(msg) \
    do { \
        printf(" FAIL: %s\n", msg); \
        tests_failed++; \
    } while(0)

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            TEST_FAIL("Assertion failed: " #cond); \
            return; \
        } \
    } while(0)

#define ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            TEST_FAIL("Assertion failed: " #ptr " is NULL"); \
            return; \
        } \
    } while(0)

#define ASSERT_EQUAL(a, b) \
    do { \
        if ((a) != (b)) { \
            TEST_FAIL("Assertion failed: " #a " != " #b); \
            return; \
        } \
    } while(0)

// ==================== Test: Initialization ====================

static void test_cli_initialization(void) {
    TEST_START("CLI Initialization");
    
    crrss_cli_context_t* ctx = crrss_cli_initialize();
    ASSERT_NOT_NULL(ctx);
    
    crrss_cli_shutdown(ctx);
    
    TEST_PASS();
}

// ==================== Test: Command Parsing ====================

static void test_command_parsing(void) {
    TEST_START("Command Parsing");
    
    crrss_cli_context_t* ctx = crrss_cli_initialize();
    ASSERT_NOT_NULL(ctx);
    
    // Test query command
    char* argv_query[] = {"crrss", "query"};
    crrss_command_t cmd = crrss_cli_parse_command(ctx, 2, argv_query);
    ASSERT_EQUAL(cmd, CMD_QUERY);
    
    // Test msm command
    char* argv_msm[] = {"crrss", "msm"};
    cmd = crrss_cli_parse_command(ctx, 2, argv_msm);
    ASSERT_EQUAL(cmd, CMD_MSM);
    
    // Test stp command
    char* argv_stp[] = {"crrss", "stp"};
    cmd = crrss_cli_parse_command(ctx, 2, argv_stp);
    ASSERT_EQUAL(cmd, CMD_STP);
    
    // Test validate command
    char* argv_validate[] = {"crrss", "validate"};
    cmd = crrss_cli_parse_command(ctx, 2, argv_validate);
    ASSERT_EQUAL(cmd, CMD_VALIDATE);
    
    // Test consult command
    char* argv_consult[] = {"crrss", "consult"};
    cmd = crrss_cli_parse_command(ctx, 2, argv_consult);
    ASSERT_EQUAL(cmd, CMD_CONSULT);
    
    // Test configure command
    char* argv_configure[] = {"crrss", "configure"};
    cmd = crrss_cli_parse_command(ctx, 2, argv_configure);
    ASSERT_EQUAL(cmd, CMD_CONFIGURE);
    
    // Test profile command
    char* argv_profile[] = {"crrss", "profile"};
    cmd = crrss_cli_parse_command(ctx, 2, argv_profile);
    ASSERT_EQUAL(cmd, CMD_PROFILE);
    
    // Test lookup command
    char* argv_lookup[] = {"crrss", "lookup"};
    cmd = crrss_cli_parse_command(ctx, 2, argv_lookup);
    ASSERT_EQUAL(cmd, CMD_LOOKUP);
    
    // Test interactive command
    char* argv_interactive[] = {"crrss", "interactive"};
    cmd = crrss_cli_parse_command(ctx, 2, argv_interactive);
    ASSERT_EQUAL(cmd, CMD_INTERACTIVE);
    
    // Test help command
    char* argv_help[] = {"crrss", "help"};
    cmd = crrss_cli_parse_command(ctx, 2, argv_help);
    ASSERT_EQUAL(cmd, CMD_HELP);
    
    // Test version command
    char* argv_version[] = {"crrss", "version"};
    cmd = crrss_cli_parse_command(ctx, 2, argv_version);
    ASSERT_EQUAL(cmd, CMD_VERSION);
    
    // Test unknown command
    char* argv_unknown[] = {"crrss", "unknown"};
    cmd = crrss_cli_parse_command(ctx, 2, argv_unknown);
    ASSERT_EQUAL(cmd, CMD_UNKNOWN);
    
    crrss_cli_shutdown(ctx);
    
    TEST_PASS();
}

// ==================== Test: Profile Management ====================

static void test_profile_management(void) {
    TEST_START("Profile Management");
    
    crrss_cli_context_t* ctx = crrss_cli_initialize();
    ASSERT_NOT_NULL(ctx);
    
    // Test listing profiles
    profile_options_t opts = {
        .list_profiles = true,
        .select_profile = NULL,
        .show_profile_info = false,
        .profile_name = NULL
    };
    
    crrss_status_t status = crrss_cli_execute_profile(ctx, &opts);
    ASSERT_EQUAL(status, CRRSS_SUCCESS);
    
    // Test selecting a profile
    opts.list_profiles = false;
    opts.select_profile = "msm";
    
    status = crrss_cli_execute_profile(ctx, &opts);
    ASSERT_EQUAL(status, CRRSS_SUCCESS);
    
    // Test showing profile info
    opts.select_profile = NULL;
    opts.show_profile_info = true;
    opts.profile_name = "msm";
    
    status = crrss_cli_execute_profile(ctx, &opts);
    ASSERT_EQUAL(status, CRRSS_SUCCESS);
    
    crrss_cli_shutdown(ctx);
    
    TEST_PASS();
}

// ==================== Test: Configuration Management ====================

static void test_configuration_management(void) {
    TEST_START("Configuration Management");
    
    crrss_cli_context_t* ctx = crrss_cli_initialize();
    ASSERT_NOT_NULL(ctx);
    
    // Test initializing configuration
    const char* test_config = "/tmp/test_crrssrc";
    configure_options_t opts = {
        .config_file = test_config,
        .show_config = false,
        .init_config = true,
        .validate_config = false,
        .set_option = NULL
    };
    
    crrss_status_t status = crrss_cli_execute_configure(ctx, &opts);
    ASSERT_EQUAL(status, CRRSS_SUCCESS);
    
    // Verify file was created
    ASSERT_TRUE(access(test_config, F_OK) == 0);
    
    // Test validating configuration
    opts.init_config = false;
    opts.validate_config = true;
    
    status = crrss_cli_execute_configure(ctx, &opts);
    ASSERT_EQUAL(status, CRRSS_SUCCESS);
    
    // Test showing configuration
    opts.validate_config = false;
    opts.show_config = true;
    
    status = crrss_cli_execute_configure(ctx, &opts);
    ASSERT_EQUAL(status, CRRSS_SUCCESS);
    
    // Clean up
    unlink(test_config);
    
    crrss_cli_shutdown(ctx);
    
    TEST_PASS();
}

// ==================== Test: Consultation Module ====================

static void test_consultation_module(void) {
    TEST_START("Consultation Module");
    
    crrss_cli_context_t* ctx = crrss_cli_initialize();
    ASSERT_NOT_NULL(ctx);
    
    // Test consultation with project type
    const char* test_config_output = "/tmp/test_consult_config";
    consult_options_t opts = {
        .project_directory = ".",
        .project_type = "kernel",
        .auto_detect = true,
        .suggest_profiles = true,
        .suggest_config = true,
        .output_config = test_config_output
    };
    
    crrss_status_t status = crrss_cli_execute_consult(ctx, &opts);
    ASSERT_EQUAL(status, CRRSS_SUCCESS);
    
    // Verify config file was created
    ASSERT_TRUE(access(test_config_output, F_OK) == 0);
    
    // Clean up
    unlink(test_config_output);
    
    crrss_cli_shutdown(ctx);
    
    TEST_PASS();
}

// ==================== Test: Bug Pattern Lookup ====================

static void test_bug_pattern_lookup(void) {
    TEST_START("Bug Pattern Lookup");
    
    crrss_cli_context_t* ctx = crrss_cli_initialize();
    ASSERT_NOT_NULL(ctx);
    
    // Test listing all patterns
    lookup_options_t opts = {
        .pattern_name = NULL,
        .pattern_id = NULL,
        .category = BUG_CATEGORY_UNKNOWN,
        .priority = BUG_PRIORITY_UNKNOWN,
        .list_all = true,
        .show_details = false
    };
    
    crrss_status_t status = crrss_cli_execute_lookup(ctx, &opts);
    ASSERT_EQUAL(status, CRRSS_SUCCESS);
    
    // Test looking up specific pattern
    opts.list_all = false;
    opts.pattern_name = "MEMORY_LEAK";
    
    status = crrss_cli_execute_lookup(ctx, &opts);
    ASSERT_EQUAL(status, CRRSS_SUCCESS);
    
    crrss_cli_shutdown(ctx);
    
    TEST_PASS();
}

// ==================== Test: Configuration File I/O ====================

static void test_configuration_file_io(void) {
    TEST_START("Configuration File I/O");
    
    crrss_cli_context_t* ctx = crrss_cli_initialize();
    ASSERT_NOT_NULL(ctx);
    
    const char* test_config = "/tmp/test_crrss_config";
    
    // Test saving configuration
    crrss_status_t status = crrss_cli_save_config(ctx, test_config);
    ASSERT_EQUAL(status, CRRSS_SUCCESS);
    
    // Verify file was created
    ASSERT_TRUE(access(test_config, F_OK) == 0);
    
    // Test loading configuration
    status = crrss_cli_load_config(ctx, test_config);
    ASSERT_EQUAL(status, CRRSS_SUCCESS);
    
    // Clean up
    unlink(test_config);
    
    crrss_cli_shutdown(ctx);
    
    TEST_PASS();
}

// ==================== Test: Help and Version ====================

static void test_help_and_version(void) {
    TEST_START("Help and Version");
    
    // These functions just print output, so we just verify they don't crash
    crrss_cli_print_help();
    crrss_cli_print_version();
    
    TEST_PASS();
}

// ==================== Test: Error Handling ====================

static void test_error_handling(void) {
    TEST_START("Error Handling");
    
    // Test NULL context handling
    crrss_status_t status = crrss_cli_execute_query(NULL, NULL);
    ASSERT_EQUAL(status, CRRSS_ERROR_NOT_INITIALIZED);
    
    status = crrss_cli_execute_stats(NULL, NULL);
    ASSERT_EQUAL(status, CRRSS_ERROR_NOT_INITIALIZED);
    
    status = crrss_cli_execute_msm(NULL, NULL);
    ASSERT_EQUAL(status, CRRSS_ERROR_NOT_INITIALIZED);
    
    status = crrss_cli_execute_validate(NULL, NULL);
    ASSERT_EQUAL(status, CRRSS_ERROR_NOT_INITIALIZED);
    
    // Test NULL options handling
    crrss_cli_context_t* ctx = crrss_cli_initialize();
    ASSERT_NOT_NULL(ctx);
    
    status = crrss_cli_execute_query(ctx, NULL);
    ASSERT_EQUAL(status, CRRSS_ERROR_INVALID_PARAM);
    
    status = crrss_cli_execute_stats(ctx, NULL);
    ASSERT_EQUAL(status, CRRSS_ERROR_INVALID_PARAM);
    
    crrss_cli_shutdown(ctx);
    
    TEST_PASS();
}

// ==================== Test: Integration ====================

static void test_integration(void) {
    TEST_START("Integration Test");
    
    crrss_cli_context_t* ctx = crrss_cli_initialize();
    ASSERT_NOT_NULL(ctx);
    
    // Create a test configuration
    configure_options_t config_opts = {
        .config_file = "/tmp/test_integration_config",
        .init_config = true
    };
    
    crrss_status_t status = crrss_cli_execute_configure(ctx, &config_opts);
    ASSERT_EQUAL(status, CRRSS_SUCCESS);
    
    // Load the configuration
    status = crrss_cli_load_config(ctx, "/tmp/test_integration_config");
    ASSERT_EQUAL(status, CRRSS_SUCCESS);
    
    // Run consultation
    consult_options_t consult_opts = {
        .project_directory = ".",
        .project_type = "kernel",
        .auto_detect = true,
        .suggest_profiles = true,
        .suggest_config = true,
        .output_config = NULL
    };
    
    status = crrss_cli_execute_consult(ctx, &consult_opts);
    ASSERT_EQUAL(status, CRRSS_SUCCESS);
    
    // List profiles
    profile_options_t profile_opts = {
        .list_profiles = true
    };
    
    status = crrss_cli_execute_profile(ctx, &profile_opts);
    ASSERT_EQUAL(status, CRRSS_SUCCESS);
    
    // Clean up
    unlink("/tmp/test_integration_config");
    
    crrss_cli_shutdown(ctx);
    
    TEST_PASS();
}

// ==================== Main Test Runner ====================

int main(void) {
    printf("==============================================\n");
    printf("CRRSS CLI Unit Tests - Phase 2 Stage 4\n");
    printf("==============================================\n\n");
    
    printf("Running CLI Tests:\n");
    
    // Run all tests
    test_cli_initialization();
    test_command_parsing();
    test_profile_management();
    test_configuration_management();
    test_consultation_module();
    test_bug_pattern_lookup();
    test_configuration_file_io();
    test_help_and_version();
    test_error_handling();
    test_integration();
    
    // Print summary
    printf("\n==============================================\n");
    printf("Test Summary:\n");
    printf("  Total:  %d\n", tests_run);
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("==============================================\n");
    
    if (tests_failed > 0) {
        printf("RESULT: FAILED\n\n");
        return 1;
    } else {
        printf("RESULT: ALL TESTS PASSED\n\n");
        return 0;
    }
}
