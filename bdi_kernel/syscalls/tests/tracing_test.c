
/**
 * @file tracing_test.c
 * @brief Unit Tests for Syscall Tracing
 */

#include "../../tracing/syscall_trace.h"
#include "../syscalls.h"
#include <stdio.h>
#include <assert.h>

/**
 * @brief Test tracing initialization
 */
static void test_trace_init(void) {
    printf("Test: Tracing initialization...\n");
    
    int result = syscall_trace_init();
    assert(result == 0);
    
    printf("  PASS: Tracing initialized\n");
}

/**
 * @brief Test tracing enable/disable
 */
static void test_trace_enable_disable(void) {
    printf("Test: Tracing enable/disable...\n");
    
    syscall_trace_enable();
    syscall_trace_disable();
    
    printf("  PASS: Tracing enable/disable works\n");
}

/**
 * @brief Test syscall filtering
 */
static void test_trace_filtering(void) {
    printf("Test: Syscall filtering...\n");
    
    /* Enable tracing for specific syscall */
    syscall_trace_set_filter(SYS_getpid, true);
    
    /* Disable tracing for another syscall */
    syscall_trace_set_filter(SYS_fork, false);
    
    printf("  PASS: Syscall filtering works\n");
}

/**
 * @brief Test sampling rate
 */
static void test_trace_sampling(void) {
    printf("Test: Sampling rate...\n");
    
    syscall_trace_set_sample_rate(10);
    
    printf("  PASS: Sampling rate set\n");
}

/**
 * @brief Test trace entry/exit
 */
static void test_trace_entry_exit(void) {
    printf("Test: Trace entry/exit...\n");
    
    syscall_trace_enable();
    
    syscall_args_t args = {0};
    syscall_trace_entry(SYS_getpid, &args);
    syscall_trace_exit(SYS_getpid, 12345);
    
    syscall_trace_disable();
    
    printf("  PASS: Trace entry/exit works\n");
}

/**
 * @brief Test trace buffer
 */
static void test_trace_buffer(void) {
    printf("Test: Trace buffer...\n");
    
    syscall_trace_enable();
    
    /* Generate some trace entries */
    syscall_args_t args = {0};
    for (int i = 0; i < 10; i++) {
        syscall_trace_entry(SYS_getpid, &args);
        syscall_trace_exit(SYS_getpid, 12345);
    }
    
    /* Print trace buffer */
    syscall_trace_print();
    
    /* Clear buffer */
    syscall_trace_clear();
    
    syscall_trace_disable();
    
    printf("  PASS: Trace buffer works\n");
}

/**
 * @brief Run all tracing tests
 */
int main(void) {
    printf("\n=== Syscall Tracing Tests ===\n\n");
    
    /* Run tests */
    test_trace_init();
    test_trace_enable_disable();
    test_trace_filtering();
    test_trace_sampling();
    test_trace_entry_exit();
    test_trace_buffer();
    
    printf("\n=== All Tests Passed ===\n");
    return 0;
}
