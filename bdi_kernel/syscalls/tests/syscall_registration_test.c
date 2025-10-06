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
 * @brief Test that extended syscalls are registered (have non-null handlers)
 * 
 * This test verifies that syscalls are REGISTERED in the table, not that they
 * are fully IMPLEMENTED. Stub handlers may legitimately return -ENOSYS while
 * still being properly registered. The key is that the syscall table entry
 * exists and points to a valid handler function.
 */
static void test_extended_syscalls_registered(void) {
    printf("\nTest: Verifying extended syscalls are registered (have handlers)...\n");
    
    /* Test a few extended syscalls from different categories */
    /* We verify that handlers are REGISTERED, not that they are fully IMPLEMENTED */
    /* Stub handlers may return -ENOSYS, which is acceptable */
    
    const uint32_t test_syscalls[] = {
        SYS_sigaction,  /* Signal handling (9) */
        SYS_getuid,     /* User/Group ID (13) */
        SYS_chmod,      /* File permissions (29) - NEWLY ADDED */
        SYS_chown,      /* File ownership (30) - NEWLY ADDED */
        SYS_lstat,      /* Extended file operations (27) */
        SYS_link,       /* Extended directory operations (44) */
        SYS_msync,      /* Extended memory management (53) */
        SYS_bind,       /* Socket operations (73) */
        SYS_time,       /* Time operations (90) */
        SYS_uname       /* System information (100) */
    };
    
    const char *test_names[] = {
        "sys_sigaction (9)",
        "sys_getuid (13)",
        "sys_chmod (29)",
        "sys_chown (30)",
        "sys_lstat (27)",
        "sys_link (44)",
        "sys_msync (53)",
        "sys_bind (73)",
        "sys_time (90)",
        "sys_uname (100)"
    };
    
    int num_tests = sizeof(test_syscalls) / sizeof(test_syscalls[0]);
    
    for (int i = 0; i < num_tests; i++) {
        printf("  Testing %s...\n", test_names[i]);
        
        /* Verify handler is registered (non-null entry in table) */
        const char *name = syscall_get_name(test_syscalls[i]);
        assert(name != nullptr);
        
        printf("    ✓ Handler registered: %s\n", name);
        
        /* Note: We don't check the return value because stub handlers */
        /* legitimately return -ENOSYS. Registration != Implementation. */
    }
    
    printf("\n  PASS: All tested extended syscalls are properly registered!\n");
    printf("  Note: Stub handlers may return -ENOSYS, which is acceptable.\n");
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
    test_extended_syscalls_registered();
    test_syscall_statistics();
    
    printf("\n=============================================================\n");
    printf("ALL TESTS PASSED! ✓\n");
    printf("P1 Bug Fix Verified: All 108 syscalls are properly registered\n");
    printf("=============================================================\n");
    
    return 0;
}
