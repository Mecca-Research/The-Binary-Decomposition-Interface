/**
 * @file syscall_registration_test.c
 * @brief Test to verify all 108 syscalls are properly registered
 * 
 * This test verifies the P1 bug fix that ensures syscall_register_extended()
 * actually registers all syscalls instead of just printing a stub message.
 */

#include "../syscalls.h"
#include "../syscall_dispatch.h"
#include <stdio.h>
#include <assert.h>
#include <errno.h>

/**
 * @brief Test that all syscalls are registered and reachable
 */
static void test_all_syscalls_registered(void) {
    printf("Test: Verifying all 108 syscalls are registered...\n");
    
    int registered_count = 0;
    int unregistered_count = 0;
    
    /* Test each syscall number */
    for (uint32_t i = 0; i < SYSCALL_COUNT; i++) {
        const char *name = syscall_get_name(i);
        
        if (name != nullptr) {
            registered_count++;
            printf("  [%3u] %-25s - REGISTERED\n", i, name);
        } else {
            unregistered_count++;
            printf("  [%3u] %-25s - MISSING!\n", i, "(unknown)");
        }
    }
    
    printf("\nSummary:\n");
    printf("  Total syscalls:        %d\n", SYSCALL_COUNT);
    printf("  Registered syscalls:   %d\n", registered_count);
    printf("  Unregistered syscalls: %d\n", unregistered_count);
    
    /* All syscalls should be registered */
    assert(registered_count == SYSCALL_COUNT);
    assert(unregistered_count == 0);
    
    printf("\n  PASS: All %d syscalls are properly registered!\n", SYSCALL_COUNT);
}

/**
 * @brief Test that extended syscalls don't return -ENOSYS
 */
static void test_extended_syscalls_reachable(void) {
    printf("\nTest: Verifying extended syscalls are reachable (not -ENOSYS)...\n");
    
    syscall_args_t args = {0};
    int64_t result;
    
    /* Test a few extended syscalls from different categories */
    
    /* Signal handling (9) */
    printf("  Testing sys_sigaction (9)...\n");
    result = syscall_dispatch(SYS_sigaction, &args);
    /* Should not be -ENOSYS, even if not fully implemented */
    assert(result != -ENOSYS);
    printf("    Result: %ld (not -ENOSYS) ✓\n", result);
    
    /* User/Group ID (13) */
    printf("  Testing sys_getuid (13)...\n");
    result = syscall_dispatch(SYS_getuid, &args);
    assert(result >= 0);  /* Should return valid UID */
    printf("    Result: %ld (valid UID) ✓\n", result);
    
    /* Extended file operations (27) */
    printf("  Testing sys_lstat (27)...\n");
    result = syscall_dispatch(SYS_lstat, &args);
    /* Should not be -ENOSYS (will be -EFAULT due to null pointer) */
    assert(result != -ENOSYS);
    printf("    Result: %ld (not -ENOSYS) ✓\n", result);
    
    /* Extended directory operations (44) */
    printf("  Testing sys_link (44)...\n");
    result = syscall_dispatch(SYS_link, &args);
    assert(result != -ENOSYS);
    printf("    Result: %ld (not -ENOSYS) ✓\n", result);
    
    /* Extended memory management (53) */
    printf("  Testing sys_msync (53)...\n");
    result = syscall_dispatch(SYS_msync, &args);
    assert(result != -ENOSYS);
    printf("    Result: %ld (not -ENOSYS) ✓\n", result);
    
    /* Socket operations (73) */
    printf("  Testing sys_bind (73)...\n");
    result = syscall_dispatch(SYS_bind, &args);
    assert(result != -ENOSYS);
    printf("    Result: %ld (not -ENOSYS) ✓\n", result);
    
    /* Time operations (90) */
    printf("  Testing sys_time (90)...\n");
    result = syscall_dispatch(SYS_time, &args);
    assert(result >= 0);  /* Should return valid time */
    printf("    Result: %ld (valid time) ✓\n", result);
    
    /* System information (100) */
    printf("  Testing sys_uname (100)...\n");
    result = syscall_dispatch(SYS_uname, &args);
    assert(result != -ENOSYS);
    printf("    Result: %ld (not -ENOSYS) ✓\n", result);
    
    printf("\n  PASS: All tested extended syscalls are reachable!\n");
}

/**
 * @brief Test syscall statistics are being tracked
 */
static void test_syscall_statistics(void) {
    printf("\nTest: Verifying syscall statistics tracking...\n");
    
    syscall_args_t args = {0};
    
    /* Call a syscall multiple times */
    for (int i = 0; i < 5; i++) {
        syscall_dispatch(SYS_getpid, &args);
    }
    
    /* Get statistics */
    const syscall_stats_t *stats = syscall_get_stats(SYS_getpid);
    assert(stats != nullptr);
    
    printf("  sys_getpid statistics:\n");
    printf("    Call count:  %lu\n", stats->call_count);
    printf("    Error count: %lu\n", stats->error_count);
    
    /* Should have at least 5 calls */
    assert(stats->call_count >= 5);
    
    printf("\n  PASS: Syscall statistics are being tracked!\n");
}

/**
 * @brief Main test function
 */
int main(void) {
    printf("=============================================================\n");
    printf("Syscall Registration Test - P1 Bug Fix Verification\n");
    printf("=============================================================\n\n");
    
    /* Initialize syscall subsystem */
    printf("Initializing syscall subsystem...\n");
    int result = syscall_init();
    assert(result == 0);
    printf("Initialization complete!\n\n");
    
    /* Run tests */
    test_all_syscalls_registered();
    test_extended_syscalls_reachable();
    test_syscall_statistics();
    
    printf("\n=============================================================\n");
    printf("ALL TESTS PASSED! ✓\n");
    printf("P1 Bug Fix Verified: All 108 syscalls are properly registered\n");
    printf("=============================================================\n");
    
    return 0;
}
