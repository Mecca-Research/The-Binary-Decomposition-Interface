
/**
 * @file syscall_dispatch_test.c
 * @brief Unit Tests for Syscall Dispatch
 */

#include "../syscalls.h"
#include "../syscall_dispatch.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

/**
 * @brief Test syscall dispatch with valid syscall
 */
static void test_dispatch_valid_syscall(void) {
    printf("Test: Dispatch valid syscall (getpid)...\n");
    
    syscall_args_t args = {0};
    int64_t result = syscall_entry(SYS_getpid, &args);
    
    assert(result >= 0);
    printf("  PASS: getpid returned %ld\n", result);
}

/**
 * @brief Test syscall dispatch with invalid syscall number
 */
static void test_dispatch_invalid_syscall(void) {
    printf("Test: Dispatch invalid syscall...\n");
    
    syscall_args_t args = {0};
    int64_t result = syscall_entry(9999, &args);
    
    assert(result == -ENOSYS);
    printf("  PASS: Invalid syscall returned -ENOSYS\n");
}

/**
 * @brief Test syscall dispatch with nullptr arguments
 */
static void test_dispatch_null_args(void) {
    printf("Test: Dispatch with nullptr arguments...\n");
    
    int64_t result = syscall_dispatch(SYS_getpid, nullptr);
    
    assert(result == -EFAULT);
    printf("  PASS: nullptr arguments returned -EFAULT\n");
}

/**
 * @brief Test 32-bit syscall entry
 */
static void test_32bit_syscall(void) {
    printf("Test: 32-bit syscall entry...\n");
    
    uint32_t args32[6] = {0};
    int32_t result = syscall_entry_32(SYS_getpid, args32);
    
    assert(result >= 0);
    printf("  PASS: 32-bit getpid returned %d\n", result);
}

/**
 * @brief Test syscall statistics
 */
static void test_syscall_stats(void) {
    printf("Test: Syscall statistics...\n");
    
    /* Call getpid a few times */
    syscall_args_t args = {0};
    for (int i = 0; i < 5; i++) {
        syscall_entry(SYS_getpid, &args);
    }
    
    const syscall_stats_t *stats = syscall_get_stats(SYS_getpid);
    assert(stats != nullptr);
    
    uint64_t call_count = atomic_load_explicit(&stats->call_count, memory_order_relaxed);
    assert(call_count >= 5);
    
    printf("  PASS: Statistics tracked correctly (call_count=%lu)\n", call_count);
}

/**
 * @brief Test syscall name lookup
 */
static void test_syscall_name(void) {
    printf("Test: Syscall name lookup...\n");
    
    const char *name = syscall_get_name(SYS_getpid);
    assert(name != nullptr);
    assert(strcmp(name, "getpid") == 0);
    
    printf("  PASS: Syscall name lookup works\n");
}

/**
 * @brief Run all syscall dispatch tests
 */
int main(void) {
    printf("\n=== Syscall Dispatch Tests ===\n\n");
    
    /* Initialize syscall subsystem */
    int result = syscall_init();
    if (result < 0) {
        fprintf(stderr, "Failed to initialize syscall subsystem\n");
        return 1;
    }
    
    /* Run tests */
    test_dispatch_valid_syscall();
    test_dispatch_invalid_syscall();
    test_dispatch_null_args();
    test_32bit_syscall();
    test_syscall_stats();
    test_syscall_name();
    
    printf("\n=== All Tests Passed ===\n");
    return 0;
}
