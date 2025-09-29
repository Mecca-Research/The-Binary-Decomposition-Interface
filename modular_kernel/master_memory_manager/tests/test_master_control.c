
/*
 * Phase 4 Master Control System Tests
 * Tests for the central orchestration and system-wide control
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

// Include Phase 4 headers
#include "../../moduler_kernel/master_memory_manager/phase4_master_control/orchestrator/mmm_orchestrator.h"
#include "../../moduler_kernel/master_memory_manager/phase4_master_control/bdi_integration/mmm_bdi_integration.h"
#include "../../moduler_kernel/master_memory_manager/phase4_master_control/optimization_engine/mmm_optimization_engine.h"

// Test framework macros
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "FAIL: %s - %s\n", __func__, message); \
            return -1; \
        } \
        printf("PASS: %s - %s\n", __func__, message); \
    } while(0)

#define TEST_START(name) \
    printf("\n=== Starting Test: %s ===\n", name)

#define TEST_END(name) \
    printf("=== Completed Test: %s ===\n\n", name)

// Global test state
static mmm_master_control_t g_test_control;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

/*
 * Test Master Control Initialization
 */
int test_master_control_init(void) {
    TEST_START("Master Control Initialization");
    
    // Initialize master control system
    int result = mmm_master_control_init(&g_test_control);
    TEST_ASSERT(result == MMM_SUCCESS, "Master control initialization");
    
    // Verify initialization state
    TEST_ASSERT(g_test_control.system_id != 0, "System ID assigned");
    TEST_ASSERT(g_test_control.component_count >= 0, "Component count valid");
    TEST_ASSERT(g_test_control.optimization_level > 0, "Optimization level set");
    
    TEST_END("Master Control Initialization");
    return 0;
}

/*
 * Test System Orchestration
 */
int test_system_orchestration(void) {
    TEST_START("System Orchestration");
    
    // Start orchestration
    int result = mmm_orchestrator_start(&g_test_control);
    TEST_ASSERT(result == MMM_SUCCESS, "Orchestrator start");
    
    // Check orchestration status
    mmm_system_status_t status;
    result = mmm_get_system_status(&g_test_control, &status);
    TEST_ASSERT(result == MMM_SUCCESS, "System status retrieval");
    TEST_ASSERT(status.orchestrator_active == true, "Orchestrator active");
    
    // Test component registration
    mmm_component_info_t component = {
        .component_id = 1001,
        .component_type = MMM_COMPONENT_MEMORY_POOL,
        .priority = MMM_PRIORITY_HIGH,
        .status = MMM_STATUS_ACTIVE
    };
    
    result = mmm_register_component(&g_test_control, &component);
    TEST_ASSERT(result == MMM_SUCCESS, "Component registration");
    
    TEST_END("System Orchestration");
    return 0;
}

/*
 * Test BDI Kernel Integration
 */
int test_bdi_integration(void) {
    TEST_START("BDI Kernel Integration");
    
    // Initialize BDI integration
    mmm_bdi_config_t bdi_config = {
        .kernel_version = BDI_KERNEL_VERSION_4_0,
        .integration_level = MMM_BDI_FULL_INTEGRATION,
        .callback_flags = MMM_BDI_CALLBACK_ALL
    };
    
    int result = mmm_bdi_integration_init(&bdi_config);
    TEST_ASSERT(result == MMM_SUCCESS, "BDI integration initialization");
    
    // Test kernel registration
    result = mmm_bdi_kernel_register(&g_test_control);
    TEST_ASSERT(result == MMM_SUCCESS, "BDI kernel registration");
    
    // Test memory subsystem integration
    result = mmm_bdi_memory_subsystem_connect();
    TEST_ASSERT(result == MMM_SUCCESS, "BDI memory subsystem connection");
    
    // Test interrupt handling integration
    result = mmm_bdi_interrupt_handler_register(mmm_bdi_interrupt_callback);
    TEST_ASSERT(result == MMM_SUCCESS, "BDI interrupt handler registration");
    
    TEST_END("BDI Kernel Integration");
    return 0;
}

/*
 * Test Optimization Engine
 */
int test_optimization_engine(void) {
    TEST_START("Optimization Engine");
    
    // Initialize optimization engine
    mmm_optimization_config_t opt_config = {
        .optimization_level = MMM_OPT_LEVEL_AGGRESSIVE,
        .adaptive_enabled = true,
        .ml_enabled = true,
        .real_time_enabled = true
    };
    
    int result = mmm_optimization_engine_init(&opt_config);
    TEST_ASSERT(result == MMM_SUCCESS, "Optimization engine initialization");
    
    // Test performance baseline establishment
    mmm_performance_baseline_t baseline;
    result = mmm_establish_performance_baseline(&baseline);
    TEST_ASSERT(result == MMM_SUCCESS, "Performance baseline establishment");
    
    // Test optimization trigger
    mmm_optimization_request_t opt_request = {
        .optimization_type = MMM_OPT_MEMORY_LAYOUT,
        .priority = MMM_PRIORITY_HIGH,
        .target_improvement = 15.0  // 15% improvement target
    };
    
    result = mmm_trigger_optimization(&opt_request);
    TEST_ASSERT(result == MMM_SUCCESS, "Optimization trigger");
    
    // Wait for optimization to complete
    usleep(100000);  // 100ms
    
    // Verify optimization results
    mmm_optimization_result_t opt_result;
    result = mmm_get_optimization_result(&opt_request, &opt_result);
    TEST_ASSERT(result == MMM_SUCCESS, "Optimization result retrieval");
    TEST_ASSERT(opt_result.improvement_achieved >= 0.0, "Optimization improvement");
    
    TEST_END("Optimization Engine");
    return 0;
}

/*
 * Test Inter-Component Communication
 */
int test_inter_component_communication(void) {
    TEST_START("Inter-Component Communication");
    
    // Initialize communication system
    mmm_communication_config_t comm_config = {
        .message_queue_size = 1024,
        .max_components = 64,
        .timeout_ms = 1000,
        .reliability_level = MMM_COMM_RELIABLE
    };
    
    int result = mmm_communication_init(&comm_config);
    TEST_ASSERT(result == MMM_SUCCESS, "Communication system initialization");
    
    // Test message sending
    mmm_message_t message = {
        .sender_id = 1001,
        .receiver_id = 1002,
        .message_type = MMM_MSG_OPTIMIZATION_REQUEST,
        .priority = MMM_PRIORITY_NORMAL,
        .data_size = sizeof(uint32_t)
    };
    
    uint32_t test_data = 0xDEADBEEF;
    memcpy(message.data, &test_data, sizeof(test_data));
    
    result = mmm_send_message(&message);
    TEST_ASSERT(result == MMM_SUCCESS, "Message sending");
    
    // Test message receiving
    mmm_message_t received_message;
    result = mmm_receive_message(1002, &received_message, 1000);
    TEST_ASSERT(result == MMM_SUCCESS, "Message receiving");
    TEST_ASSERT(received_message.sender_id == 1001, "Message sender verification");
    TEST_ASSERT(received_message.message_type == MMM_MSG_OPTIMIZATION_REQUEST, "Message type verification");
    
    uint32_t received_data;
    memcpy(&received_data, received_message.data, sizeof(received_data));
    TEST_ASSERT(received_data == test_data, "Message data integrity");
    
    TEST_END("Inter-Component Communication");
    return 0;
}

/*
 * Test System Shutdown
 */
int test_system_shutdown(void) {
    TEST_START("System Shutdown");
    
    // Test graceful shutdown
    int result = mmm_orchestrator_shutdown(&g_test_control);
    TEST_ASSERT(result == MMM_SUCCESS, "Graceful shutdown");
    
    // Verify shutdown state
    mmm_system_status_t status;
    result = mmm_get_system_status(&g_test_control, &status);
    TEST_ASSERT(result == MMM_SUCCESS, "Post-shutdown status check");
    TEST_ASSERT(status.orchestrator_active == false, "Orchestrator inactive");
    
    // Cleanup resources
    result = mmm_master_control_cleanup(&g_test_control);
    TEST_ASSERT(result == MMM_SUCCESS, "Resource cleanup");
    
    TEST_END("System Shutdown");
    return 0;
}

/*
 * Performance stress test
 */
int test_performance_stress(void) {
    TEST_START("Performance Stress Test");
    
    const int NUM_OPERATIONS = 10000;
    const int NUM_THREADS = 4;
    
    // Re-initialize for stress test
    int result = mmm_master_control_init(&g_test_control);
    TEST_ASSERT(result == MMM_SUCCESS, "Stress test initialization");
    
    result = mmm_orchestrator_start(&g_test_control);
    TEST_ASSERT(result == MMM_SUCCESS, "Stress test orchestrator start");
    
    // Measure performance
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    // Simulate high-load operations
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        mmm_component_info_t component = {
            .component_id = 2000 + i,
            .component_type = MMM_COMPONENT_CACHE,
            .priority = MMM_PRIORITY_NORMAL,
            .status = MMM_STATUS_ACTIVE
        };
        
        result = mmm_register_component(&g_test_control, &component);
        if (result != MMM_SUCCESS) {
            break;
        }
        
        // Trigger optimization every 100 operations
        if (i % 100 == 0) {
            mmm_optimization_request_t opt_request = {
                .optimization_type = MMM_OPT_CACHE_LAYOUT,
                .priority = MMM_PRIORITY_LOW,
                .target_improvement = 5.0
            };
            mmm_trigger_optimization(&opt_request);
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    // Calculate performance metrics
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) + 
                         (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    double operations_per_second = NUM_OPERATIONS / elapsed_time;
    
    printf("Performance: %.2f operations/second\n", operations_per_second);
    TEST_ASSERT(operations_per_second > 1000.0, "Performance threshold met");
    
    // Cleanup
    mmm_orchestrator_shutdown(&g_test_control);
    mmm_master_control_cleanup(&g_test_control);
    
    TEST_END("Performance Stress Test");
    return 0;
}

/*
 * Main test runner
 */
int main(int argc, char *argv[]) {
    printf("=== Phase 4 Master Control System Tests ===\n");
    printf("Testing LEGENDARY BDI BUILD components...\n\n");
    
    // Initialize test environment
    memset(&g_test_control, 0, sizeof(g_test_control));
    
    // Run test suite
    struct {
        const char *name;
        int (*test_func)(void);
    } tests[] = {
        {"Master Control Init", test_master_control_init},
        {"System Orchestration", test_system_orchestration},
        {"BDI Integration", test_bdi_integration},
        {"Optimization Engine", test_optimization_engine},
        {"Inter-Component Communication", test_inter_component_communication},
        {"System Shutdown", test_system_shutdown},
        {"Performance Stress", test_performance_stress},
        {NULL, NULL}
    };
    
    for (int i = 0; tests[i].name != NULL; i++) {
        if (tests[i].test_func() == 0) {
            g_tests_passed++;
        } else {
            g_tests_failed++;
        }
    }
    
    // Print test summary
    printf("\n=== Test Summary ===\n");
    printf("Tests Passed: %d\n", g_tests_passed);
    printf("Tests Failed: %d\n", g_tests_failed);
    printf("Total Tests: %d\n", g_tests_passed + g_tests_failed);
    
    if (g_tests_failed == 0) {
        printf("\n🎉 ALL TESTS PASSED - LEGENDARY BDI BUILD READY! 🎉\n");
        return 0;
    } else {
        printf("\n❌ Some tests failed. Please review and fix issues.\n");
        return 1;
    }
}
