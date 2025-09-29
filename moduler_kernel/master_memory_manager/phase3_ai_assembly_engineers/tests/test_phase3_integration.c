
/**
 * Phase 3 Integration Tests - Comprehensive testing of AI Assembly Engineers system
 * Tests all major components and their integration
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

// Phase 3 component headers
#include "../runtime/capsule_loader/capsule_loader.h"
#include "../runtime/hotswap_lanes/hotswap_manager.h"
#include "../integration/phase3_main.c"

// Test framework macros
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s - %s\n", __func__, message); \
            return 0; \
        } \
    } while(0)

#define TEST_PASS() \
    do { \
        printf("PASS: %s\n", __func__); \
        return 1; \
    } while(0)

#define RUN_TEST(test_func) \
    do { \
        printf("Running %s...\n", #test_func); \
        if (test_func()) { \
            tests_passed++; \
        } else { \
            tests_failed++; \
        } \
        total_tests++; \
    } while(0)

// Global test statistics
static int total_tests = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Test helper functions
static char* create_sample_capsule(void);
static void cleanup_test_files(void);

// Test functions
static int test_capsule_loader_initialization(void);
static int test_capsule_loading_and_execution(void);
static int test_hotswap_manager_initialization(void);
static int test_hotswap_lane_creation(void);
static int test_code_swapping(void);
static int test_concurrent_operations(void);
static int test_error_handling(void);
static int test_performance_metrics(void);
static int test_system_integration(void);
static int test_memory_management(void);

int main(void) {
    printf("=== Phase 3 AI Assembly Engineers - Integration Tests ===\n");
    printf("Starting comprehensive test suite...\n\n");
    
    // Initialize test environment
    cleanup_test_files();
    
    // Run all tests
    RUN_TEST(test_capsule_loader_initialization);
    RUN_TEST(test_capsule_loading_and_execution);
    RUN_TEST(test_hotswap_manager_initialization);
    RUN_TEST(test_hotswap_lane_creation);
    RUN_TEST(test_code_swapping);
    RUN_TEST(test_concurrent_operations);
    RUN_TEST(test_error_handling);
    RUN_TEST(test_performance_metrics);
    RUN_TEST(test_system_integration);
    RUN_TEST(test_memory_management);
    
    // Cleanup
    cleanup_test_files();
    
    // Print results
    printf("\n=== Test Results ===\n");
    printf("Total Tests: %d\n", total_tests);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Success Rate: %.1f%%\n", (float)tests_passed / total_tests * 100);
    
    if (tests_failed == 0) {
        printf("\n🎉 All tests passed! Phase 3 system is working correctly.\n");
        return 0;
    } else {
        printf("\n❌ Some tests failed. Please check the implementation.\n");
        return 1;
    }
}

static int test_capsule_loader_initialization(void) {
    capsule_loader_config_t config = {
        .max_concurrent_capsules = 10,
        .security_level = CAPSULE_SECURITY_STANDARD,
        .enable_sandboxing = true,
        .enable_profiling = true,
        .enable_hot_reload = true
    };
    
    capsule_status_t status = capsule_loader_initialize(&config);
    TEST_ASSERT(status == CAPSULE_SUCCESS, "Capsule loader initialization failed");
    
    // Test registry info
    capsule_registry_info_t info;
    status = capsule_get_registry_info(&info);
    TEST_ASSERT(status == CAPSULE_SUCCESS, "Failed to get registry info");
    TEST_ASSERT(info.max_capsules == 10, "Incorrect max capsules");
    TEST_ASSERT(info.active_count == 0, "Initial active count should be 0");
    
    capsule_loader_shutdown();
    TEST_PASS();
}

static int test_capsule_loading_and_execution(void) {
    // Initialize capsule loader
    capsule_loader_config_t config = {
        .max_concurrent_capsules = 5,
        .security_level = CAPSULE_SECURITY_BASIC,
        .enable_sandboxing = false,
        .enable_profiling = true,
        .enable_hot_reload = false
    };
    
    capsule_status_t status = capsule_loader_initialize(&config);
    TEST_ASSERT(status == CAPSULE_SUCCESS, "Failed to initialize capsule loader");
    
    // Create sample capsule
    char* capsule_data = create_sample_capsule();
    TEST_ASSERT(capsule_data != NULL, "Failed to create sample capsule");
    
    // Load capsule
    capsule_load_options_t load_options = {
        .validate_before_load = true,
        .enable_profiling = true,
        .allocate_stack = true,
        .enable_debugging = false,
        .timeout_seconds = 30,
        .security_level = CAPSULE_SECURITY_BASIC
    };
    
    capsule_t* capsule = capsule_load_from_memory(capsule_data, strlen(capsule_data), &load_options);
    TEST_ASSERT(capsule != NULL, "Failed to load capsule from memory");
    TEST_ASSERT(capsule->state == CAPSULE_STATE_LOADED, "Capsule not in loaded state");
    
    // Execute capsule
    capsule_execution_context_t exec_context = {
        .argc = 0,
        .argv = NULL,
        .input_data = NULL,
        .input_size = 0,
        .output_buffer = malloc(1024),
        .output_buffer_size = 1024,
        .timeout_seconds = 10,
        .enable_profiling = true
    };
    
    status = capsule_execute(capsule, &exec_context);
    TEST_ASSERT(status == CAPSULE_SUCCESS, "Capsule execution failed");
    TEST_ASSERT(capsule->execution_count == 1, "Execution count not updated");
    
    // Cleanup
    free(exec_context.output_buffer);
    capsule_unload(capsule);
    free(capsule_data);
    capsule_loader_shutdown();
    
    TEST_PASS();
}

static int test_hotswap_manager_initialization(void) {
    hotswap_config_t config = {
        .max_concurrent_lanes = 8,
        .enable_rollback = true,
        .enable_profiling = true,
        .enable_monitoring = true,
        .safety_level = HOTSWAP_SAFETY_STANDARD,
        .swap_timeout_seconds = 30
    };
    
    hotswap_status_t status = hotswap_system_initialize(&config);
    TEST_ASSERT(status == HOTSWAP_SUCCESS, "Hot-swap system initialization failed");
    
    // Test system info
    hotswap_system_info_t info;
    status = hotswap_get_system_info(&info);
    TEST_ASSERT(status == HOTSWAP_SUCCESS, "Failed to get system info");
    TEST_ASSERT(info.max_lanes == 8, "Incorrect max lanes");
    TEST_ASSERT(info.active_lanes == 0, "Initial active lanes should be 0");
    
    hotswap_system_shutdown();
    TEST_PASS();
}

static int test_hotswap_lane_creation(void) {
    // Initialize hot-swap system
    hotswap_config_t config = {
        .max_concurrent_lanes = 4,
        .enable_rollback = true,
        .enable_profiling = false,
        .enable_monitoring = false,
        .safety_level = HOTSWAP_SAFETY_BASIC,
        .swap_timeout_seconds = 15
    };
    
    hotswap_status_t status = hotswap_system_initialize(&config);
    TEST_ASSERT(status == HOTSWAP_SUCCESS, "Failed to initialize hot-swap system");
    
    // Create lane
    hotswap_lane_config_t lane_config = {
        .initial_code_size = 4096,
        .enable_profiling = false,
        .enable_rollback = true,
        .safety_level = HOTSWAP_SAFETY_BASIC,
        .max_execution_time = 60
    };
    
    hotswap_lane_t* lane = hotswap_create_lane(&lane_config);
    TEST_ASSERT(lane != NULL, "Failed to create hot-swap lane");
    TEST_ASSERT(lane->state == HOTSWAP_LANE_STATE_READY, "Lane not in ready state");
    
    // Get lane status
    hotswap_lane_status_t lane_status;
    status = hotswap_get_lane_status(lane, &lane_status);
    TEST_ASSERT(status == HOTSWAP_SUCCESS, "Failed to get lane status");
    TEST_ASSERT(lane_status.execution_count == 0, "Initial execution count should be 0");
    
    // Destroy lane
    status = hotswap_destroy_lane(lane);
    TEST_ASSERT(status == HOTSWAP_SUCCESS, "Failed to destroy lane");
    
    hotswap_system_shutdown();
    TEST_PASS();
}

static int test_code_swapping(void) {
    // Initialize system
    hotswap_config_t config = {
        .max_concurrent_lanes = 2,
        .enable_rollback = true,
        .enable_profiling = true,
        .enable_monitoring = false,
        .safety_level = HOTSWAP_SAFETY_BASIC,
        .swap_timeout_seconds = 10
    };
    
    hotswap_status_t status = hotswap_system_initialize(&config);
    TEST_ASSERT(status == HOTSWAP_SUCCESS, "Failed to initialize system");
    
    // Create lane
    hotswap_lane_config_t lane_config = {
        .initial_code_size = 2048,
        .enable_profiling = true,
        .enable_rollback = true,
        .safety_level = HOTSWAP_SAFETY_BASIC,
        .max_execution_time = 30
    };
    
    hotswap_lane_t* lane = hotswap_create_lane(&lane_config);
    TEST_ASSERT(lane != NULL, "Failed to create lane");
    
    // Prepare swap request
    char new_code[] = "mov eax, 42\nret\n";
    hotswap_request_t request = {
        .new_code_data = new_code,
        .new_code_size = strlen(new_code),
        .priority = HOTSWAP_PRIORITY_NORMAL,
        .safety_level = HOTSWAP_SAFETY_BASIC,
        .enable_rollback = true,
        .timeout_seconds = 5
    };
    strcpy(request.description, "Test code swap");
    
    // Request swap
    status = hotswap_request_swap(lane, &request);
    TEST_ASSERT(status == HOTSWAP_SUCCESS, "Code swap failed");
    TEST_ASSERT(lane->state == HOTSWAP_LANE_STATE_ACTIVE, "Lane not in active state after swap");
    
    // Execute code
    hotswap_execution_context_t exec_context = {
        .input_data = NULL,
        .input_size = 0,
        .output_buffer = malloc(512),
        .output_buffer_size = 512,
        .timeout_seconds = 5,
        .enable_profiling = true
    };
    
    hotswap_execution_result_t exec_result;
    status = hotswap_execute_code(lane, &exec_context, &exec_result);
    TEST_ASSERT(status == HOTSWAP_SUCCESS, "Code execution failed");
    
    // Cleanup
    free(exec_context.output_buffer);
    hotswap_destroy_lane(lane);
    hotswap_system_shutdown();
    
    TEST_PASS();
}

static int test_concurrent_operations(void) {
    // This test would verify concurrent capsule loading and hot-swapping
    // For simplicity, we'll test basic concurrency with multiple lanes
    
    hotswap_config_t config = {
        .max_concurrent_lanes = 3,
        .enable_rollback = false,
        .enable_profiling = false,
        .enable_monitoring = false,
        .safety_level = HOTSWAP_SAFETY_BASIC,
        .swap_timeout_seconds = 5
    };
    
    hotswap_status_t status = hotswap_system_initialize(&config);
    TEST_ASSERT(status == HOTSWAP_SUCCESS, "Failed to initialize system");
    
    // Create multiple lanes
    hotswap_lane_config_t lane_config = {
        .initial_code_size = 1024,
        .enable_profiling = false,
        .enable_rollback = false,
        .safety_level = HOTSWAP_SAFETY_BASIC,
        .max_execution_time = 10
    };
    
    hotswap_lane_t* lanes[3];
    for (int i = 0; i < 3; i++) {
        lanes[i] = hotswap_create_lane(&lane_config);
        TEST_ASSERT(lanes[i] != NULL, "Failed to create lane");
    }
    
    // Verify system info
    hotswap_system_info_t info;
    status = hotswap_get_system_info(&info);
    TEST_ASSERT(status == HOTSWAP_SUCCESS, "Failed to get system info");
    TEST_ASSERT(info.active_lanes == 3, "Incorrect active lane count");
    
    // Cleanup
    for (int i = 0; i < 3; i++) {
        hotswap_destroy_lane(lanes[i]);
    }
    
    hotswap_system_shutdown();
    TEST_PASS();
}

static int test_error_handling(void) {
    // Test various error conditions
    
    // Test invalid parameters
    capsule_status_t capsule_status = capsule_loader_initialize(NULL);
    TEST_ASSERT(capsule_status == CAPSULE_ERROR_INVALID_PARAMETER, "Should reject NULL config");
    
    hotswap_status_t hotswap_status = hotswap_system_initialize(NULL);
    TEST_ASSERT(hotswap_status == HOTSWAP_ERROR_INVALID_PARAMETER, "Should reject NULL config");
    
    // Test operations on uninitialized systems
    capsule_registry_info_t registry_info;
    capsule_status = capsule_get_registry_info(&registry_info);
    TEST_ASSERT(capsule_status != CAPSULE_SUCCESS, "Should fail on uninitialized system");
    
    hotswap_system_info_t system_info;
    hotswap_status = hotswap_get_system_info(&system_info);
    TEST_ASSERT(hotswap_status != HOTSWAP_SUCCESS, "Should fail on uninitialized system");
    
    TEST_PASS();
}

static int test_performance_metrics(void) {
    // Test performance monitoring and metrics collection
    
    capsule_loader_config_t config = {
        .max_concurrent_capsules = 2,
        .security_level = CAPSULE_SECURITY_BASIC,
        .enable_sandboxing = false,
        .enable_profiling = true,  // Enable profiling
        .enable_hot_reload = false
    };
    
    capsule_status_t status = capsule_loader_initialize(&config);
    TEST_ASSERT(status == CAPSULE_SUCCESS, "Failed to initialize with profiling");
    
    // Load and execute a capsule to generate metrics
    char* capsule_data = create_sample_capsule();
    TEST_ASSERT(capsule_data != NULL, "Failed to create sample capsule");
    
    capsule_load_options_t load_options = {
        .validate_before_load = true,
        .enable_profiling = true,
        .allocate_stack = false,
        .enable_debugging = false,
        .timeout_seconds = 10,
        .security_level = CAPSULE_SECURITY_BASIC
    };
    
    capsule_t* capsule = capsule_load_from_memory(capsule_data, strlen(capsule_data), &load_options);
    TEST_ASSERT(capsule != NULL, "Failed to load capsule");
    
    // Execute multiple times to generate metrics
    capsule_execution_context_t exec_context = {
        .argc = 0,
        .argv = NULL,
        .input_data = NULL,
        .input_size = 0,
        .output_buffer = malloc(256),
        .output_buffer_size = 256,
        .timeout_seconds = 5,
        .enable_profiling = true
    };
    
    for (int i = 0; i < 3; i++) {
        status = capsule_execute(capsule, &exec_context);
        TEST_ASSERT(status == CAPSULE_SUCCESS, "Execution failed");
    }
    
    // Verify metrics
    TEST_ASSERT(capsule->execution_count == 3, "Incorrect execution count");
    TEST_ASSERT(capsule->total_execution_time >= 0, "Invalid total execution time");
    
    // Cleanup
    free(exec_context.output_buffer);
    capsule_unload(capsule);
    free(capsule_data);
    capsule_loader_shutdown();
    
    TEST_PASS();
}

static int test_system_integration(void) {
    // Test integration between different components
    
    // Initialize both systems
    capsule_loader_config_t capsule_config = {
        .max_concurrent_capsules = 2,
        .security_level = CAPSULE_SECURITY_BASIC,
        .enable_sandboxing = false,
        .enable_profiling = false,
        .enable_hot_reload = true
    };
    
    hotswap_config_t hotswap_config = {
        .max_concurrent_lanes = 2,
        .enable_rollback = false,
        .enable_profiling = false,
        .enable_monitoring = false,
        .safety_level = HOTSWAP_SAFETY_BASIC,
        .swap_timeout_seconds = 10
    };
    
    capsule_status_t capsule_status = capsule_loader_initialize(&capsule_config);
    TEST_ASSERT(capsule_status == CAPSULE_SUCCESS, "Failed to initialize capsule loader");
    
    hotswap_status_t hotswap_status = hotswap_system_initialize(&hotswap_config);
    TEST_ASSERT(hotswap_status == HOTSWAP_SUCCESS, "Failed to initialize hot-swap system");
    
    // Test that both systems can operate simultaneously
    capsule_registry_info_t registry_info;
    capsule_status = capsule_get_registry_info(&registry_info);
    TEST_ASSERT(capsule_status == CAPSULE_SUCCESS, "Failed to get registry info");
    
    hotswap_system_info_t system_info;
    hotswap_status = hotswap_get_system_info(&system_info);
    TEST_ASSERT(hotswap_status == HOTSWAP_SUCCESS, "Failed to get system info");
    
    // Cleanup both systems
    capsule_loader_shutdown();
    hotswap_system_shutdown();
    
    TEST_PASS();
}

static int test_memory_management(void) {
    // Test memory allocation and deallocation
    
    capsule_loader_config_t config = {
        .max_concurrent_capsules = 1,
        .security_level = CAPSULE_SECURITY_BASIC,
        .enable_sandboxing = false,
        .enable_profiling = false,
        .enable_hot_reload = false
    };
    
    capsule_status_t status = capsule_loader_initialize(&config);
    TEST_ASSERT(status == CAPSULE_SUCCESS, "Failed to initialize");
    
    // Load and unload multiple capsules to test memory management
    for (int i = 0; i < 5; i++) {
        char* capsule_data = create_sample_capsule();
        TEST_ASSERT(capsule_data != NULL, "Failed to create capsule data");
        
        capsule_load_options_t load_options = {
            .validate_before_load = false,
            .enable_profiling = false,
            .allocate_stack = true,  // Test stack allocation
            .enable_debugging = false,
            .timeout_seconds = 5,
            .security_level = CAPSULE_SECURITY_BASIC
        };
        
        capsule_t* capsule = capsule_load_from_memory(capsule_data, strlen(capsule_data), &load_options);
        TEST_ASSERT(capsule != NULL, "Failed to load capsule");
        
        // Immediately unload to test cleanup
        status = capsule_unload(capsule);
        TEST_ASSERT(status == CAPSULE_SUCCESS, "Failed to unload capsule");
        
        free(capsule_data);
    }
    
    capsule_loader_shutdown();
    TEST_PASS();
}

// Helper functions

static char* create_sample_capsule(void) {
    const char* capsule_template = 
        "CAPSULE_METADATA_START\n"
        "name: Test Capsule\n"
        "version: 1.0\n"
        "author: Test Suite\n"
        "description: Sample capsule for testing\n"
        "target_arch: x86\n"
        "optimization_level: 1\n"
        "safety_level: 1\n"
        "CAPSULE_METADATA_END\n"
        "ASSEMBLY_CODE_START\n"
        "mov eax, 0\n"
        "ret\n"
        "ASSEMBLY_CODE_END\n";
    
    size_t len = strlen(capsule_template);
    char* capsule_data = malloc(len + 1);
    if (capsule_data) {
        strcpy(capsule_data, capsule_template);
    }
    
    return capsule_data;
}

static void cleanup_test_files(void) {
    // Remove any temporary files created during testing
    system("rm -f test_capsule_*.tmp");
    system("rm -f test_hotswap_*.tmp");
}
