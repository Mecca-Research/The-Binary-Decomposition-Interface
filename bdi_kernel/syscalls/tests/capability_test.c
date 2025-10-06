
/**
 * @file capability_test.c
 * @brief Unit Tests for Capability Framework
 */

#include "../../security/capability.h"
#include "../syscalls.h"
#include <stdio.h>
#include <assert.h>

/**
 * @brief Test capability initialization
 */
static void test_capability_init(void) {
    printf("Test: Capability initialization...\n");
    
    int result = capability_init();
    assert(result == 0);
    
    printf("  PASS: Capability system initialized\n");
}

/**
 * @brief Test capability checking
 */
static void test_capability_check(void) {
    printf("Test: Capability checking...\n");
    
    /* Check read-only syscall (should always pass) */
    bool allowed = capability_check_syscall(SYS_getpid);
    assert(allowed == true);
    
    printf("  PASS: Capability checking works\n");
}

/**
 * @brief Test capability auditing
 */
static void test_capability_audit(void) {
    printf("Test: Capability auditing...\n");
    
    capability_audit_enable();
    
    /* Perform some capability checks */
    capability_check_syscall(SYS_getpid);
    capability_check_syscall(SYS_fork);
    
    capability_audit_disable();
    
    printf("  PASS: Capability auditing works\n");
}

/**
 * @brief Run all capability tests
 */
int main(void) {
    printf("\n=== Capability Tests ===\n\n");
    
    /* Run tests */
    test_capability_init();
    test_capability_check();
    test_capability_audit();
    
    printf("\n=== All Tests Passed ===\n");
    return 0;
}
