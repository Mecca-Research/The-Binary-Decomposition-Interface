
/**
 * @file test_memory.c
 * @brief Tests for Memory Integration Layer
 */

#include "../memory_layer/memory_integration.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

void test_memory_initialization() {
    printf("Testing Memory Integration initialization...\n");
    
    memory_integration_config_t config = {
        .enable_leak_detection = true,
        .enable_use_after_free_detection = true,
        .enable_double_free_detection = true,
        .track_allocations = true,
        .max_tracked_allocations = 1000,
        .memory_subsystem_path = NULL
    };
    
    memory_integration_context_t* ctx = memory_integration_initialize(&config);
    assert(ctx != NULL);
    
    memory_integration_shutdown(ctx);
    printf("  ✓ Memory Integration initialization test passed\n");
}

void test_memory_tracking() {
    printf("Testing memory allocation tracking...\n");
    
    memory_integration_config_t config = {
        .enable_leak_detection = true,
        .enable_use_after_free_detection = true,
        .enable_double_free_detection = true,
        .track_allocations = true,
        .max_tracked_allocations = 1000,
        .memory_subsystem_path = NULL
    };
    
    memory_integration_context_t* ctx = memory_integration_initialize(&config);
    assert(ctx != NULL);
    
    // Track an allocation
    void* fake_addr = (void*)0x1000;
    crrss_status_t status = memory_integration_track_allocation(
        ctx, fake_addr, 100, "test.c:42"
    );
    assert(status == CRRSS_SUCCESS);
    
    // Track deallocation
    status = memory_integration_track_deallocation(ctx, fake_addr);
    assert(status == CRRSS_SUCCESS);
    
    memory_integration_shutdown(ctx);
    printf("  ✓ Memory tracking test passed\n");
}

void test_leak_detection() {
    printf("Testing memory leak detection...\n");
    
    memory_integration_config_t config = {
        .enable_leak_detection = true,
        .enable_use_after_free_detection = true,
        .enable_double_free_detection = true,
        .track_allocations = true,
        .max_tracked_allocations = 1000,
        .memory_subsystem_path = NULL
    };
    
    memory_integration_context_t* ctx = memory_integration_initialize(&config);
    assert(ctx != NULL);
    
    // Track allocations without freeing
    void* addr1 = (void*)0x1000;
    void* addr2 = (void*)0x2000;
    
    memory_integration_track_allocation(ctx, addr1, 100, "test.c:10");
    memory_integration_track_allocation(ctx, addr2, 200, "test.c:20");
    
    // Detect leaks
    leak_detection_report_t report = {0};
    report.max_records = 10;
    
    crrss_status_t status = memory_integration_detect_leaks(ctx, &report);
    assert(status == CRRSS_SUCCESS);
    
    printf("  Potential leaks: %u\n", report.potential_leaks);
    printf("  Leaked bytes: %lu\n", report.total_leaked_bytes);
    
    if (report.leak_records) {
        free(report.leak_records);
    }
    
    memory_integration_shutdown(ctx);
    printf("  ✓ Leak detection test passed\n");
}

void test_memory_statistics() {
    printf("Testing memory statistics...\n");
    
    memory_integration_config_t config = {
        .enable_leak_detection = true,
        .enable_use_after_free_detection = true,
        .enable_double_free_detection = true,
        .track_allocations = true,
        .max_tracked_allocations = 1000,
        .memory_subsystem_path = NULL
    };
    
    memory_integration_context_t* ctx = memory_integration_initialize(&config);
    assert(ctx != NULL);
    
    uint64_t total_allocs = 0;
    uint64_t total_frees = 0;
    uint64_t current_usage = 0;
    
    crrss_status_t status = memory_integration_get_statistics(
        ctx, MEMORY_SUBSYSTEM_HAM,
        &total_allocs, &total_frees, &current_usage
    );
    assert(status == CRRSS_SUCCESS);
    
    printf("  Total allocations: %lu\n", total_allocs);
    printf("  Total frees: %lu\n", total_frees);
    printf("  Current usage: %lu\n", current_usage);
    
    memory_integration_shutdown(ctx);
    printf("  ✓ Memory statistics test passed\n");
}

int main(void) {
    printf("=== Running Memory Integration Tests ===\n\n");
    
    test_memory_initialization();
    test_memory_tracking();
    test_leak_detection();
    test_memory_statistics();
    
    printf("\n=== All Memory Integration Tests Passed ===\n");
    return 0;
}
