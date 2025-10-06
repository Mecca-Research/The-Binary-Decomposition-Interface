
/**
 * @file integration_test.c
 * @brief Integration Tests for Complete System Services Layer
 */

#include "../syscalls.h"
#include "../syscall_dispatch.h"
#include "../vdso.h"
#include "../../tracing/syscall_trace.h"
#include "../../security/capability.h"
#include <stdio.h>
#include <assert.h>

/**
 * @brief Test complete syscall flow with all subsystems
 */
static void test_complete_syscall_flow(void) {
    printf("Test: Complete syscall flow...\n");
    
    /* Enable tracing and auditing */
    syscall_trace_enable();
    capability_audit_enable();
    
    /* Execute a syscall through the full stack */
    syscall_args_t args = {0};
    int64_t result = syscall_entry(SYS_getpid, &args);
    
    assert(result >= 0);
    
    /* Disable tracing and auditing */
    syscall_trace_disable();
    capability_audit_disable();
    
    printf("  PASS: Complete syscall flow works (result=%ld)\n", result);
}

/**
 * @brief Test vDSO fast path
 */
static void test_vdso_fast_path(void) {
    printf("Test: vDSO fast path...\n");
    
    /* Update vDSO data */
    vdso_update_process_info(12345, 12345, 1, 0);
    vdso_update_time_complete();
    
    /* Call vDSO syscalls */
    syscall_args_t args = {0};
    int64_t pid = sys_vdso_getpid(&args);
    
    assert(pid == 12345);
    
    printf("  PASS: vDSO fast path works (pid=%ld)\n", pid);
}

/**
 * @brief Test syscall batching
 */
static void test_syscall_batching(void) {
    printf("Test: Syscall batching...\n");
    
    /* Create a batch of syscalls */
    struct {
        uint32_t syscall_num;
        syscall_args_t args;
        int64_t result;
    } calls[3] = {
        {SYS_getpid, {0}, 0},
        {SYS_getppid, {0}, 0},
        {SYS_getpagesize, {0}, 0}
    };
    
    batch_params_t batch = {
        .count = 3,
        .flags = 0,
        .calls = calls
    };
    
    syscall_args_t args = {
        .arg0 = (uint64_t)&batch
    };
    
    int64_t result = sys_batch(&args);
    
    assert(result >= 0);
    
    printf("  PASS: Syscall batching works (completed=%ld)\n", result);
}

/**
 * @brief Test syscall statistics
 */
static void test_syscall_statistics(void) {
    printf("Test: Syscall statistics...\n");
    
    /* Execute several syscalls */
    syscall_args_t args = {0};
    for (int i = 0; i < 100; i++) {
        syscall_entry(SYS_getpid, &args);
    }
    
    /* Get statistics */
    const syscall_stats_t *stats = syscall_get_stats(SYS_getpid);
    assert(stats != nullptr);
    
    uint64_t call_count = atomic_load_explicit(&stats->call_count, memory_order_relaxed);
    assert(call_count >= 100);
    
    printf("  PASS: Statistics tracking works (calls=%lu)\n", call_count);
}

/**
 * @brief Test error handling
 */
static void test_error_handling(void) {
    printf("Test: Error handling...\n");
    
    /* Test invalid syscall number */
    syscall_args_t args = {0};
    int64_t result = syscall_entry(9999, &args);
    assert(result == -ENOSYS);
    
    /* Test nullptr arguments */
    result = syscall_dispatch(SYS_getpid, nullptr);
    assert(result == -EFAULT);
    
    printf("  PASS: Error handling works\n");
}

/**
 * @brief Run all integration tests
 */
int main(void) {
    printf("\n=== System Services Integration Tests ===\n\n");
    
    /* Initialize all subsystems */
    printf("Initializing subsystems...\n");
    
    int result = syscall_init();
    assert(result == 0);
    
    result = vdso_init_complete();
    assert(result == 0);
    
    result = syscall_trace_init();
    assert(result == 0);
    
    result = capability_init();
    assert(result == 0);
    
    printf("All subsystems initialized\n\n");
    
    /* Run tests */
    test_complete_syscall_flow();
    test_vdso_fast_path();
    test_syscall_batching();
    test_syscall_statistics();
    test_error_handling();
    
    /* Print statistics */
    printf("\n=== Final Statistics ===\n");
    syscall_print_stats();
    
    printf("\n=== All Integration Tests Passed ===\n");
    return 0;
}
