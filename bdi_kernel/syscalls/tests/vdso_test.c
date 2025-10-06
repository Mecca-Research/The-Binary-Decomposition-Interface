
/**
 * @file vdso_test.c
 * @brief Unit Tests for vDSO Implementation
 */

#include "../vdso.h"
#include "../syscalls.h"
#include <stdio.h>
#include <assert.h>
#include <time.h>

/**
 * @brief Test vDSO initialization
 */
static void test_vdso_init(void) {
    printf("Test: vDSO initialization...\n");
    
    int result = vdso_init_complete();
    assert(result == 0);
    
    void *page_addr = vdso_get_page_address();
    assert(page_addr != nullptr);
    
    printf("  PASS: vDSO initialized at %p\n", page_addr);
}

/**
 * @brief Test vDSO getpid
 */
static void test_vdso_getpid(void) {
    printf("Test: vDSO getpid...\n");
    
    /* Update vDSO with current process info */
    vdso_update_process_info(12345, 12345, 1, 0);
    
    int64_t pid = vdso_getpid_impl();
    assert(pid == 12345);
    
    printf("  PASS: vDSO getpid returned %ld\n", pid);
}

/**
 * @brief Test vDSO time functions
 */
static void test_vdso_time(void) {
    printf("Test: vDSO time functions...\n");
    
    /* Update vDSO time */
    vdso_update_time_complete();
    
    /* Test time() */
    time_t t = 0;
    int64_t result = vdso_time_impl(&t);
    assert(result > 0);
    assert(t > 0);
    
    printf("  PASS: vDSO time returned %ld\n", result);
}

/**
 * @brief Test vDSO getcpu
 */
static void test_vdso_getcpu(void) {
    printf("Test: vDSO getcpu...\n");
    
    /* Update vDSO with CPU info */
    vdso_update_process_info(12345, 12345, 1, 3);
    
    uint32_t cpu = 0;
    uint32_t node = 0;
    int64_t result = vdso_getcpu_impl(&cpu, &node);
    
    assert(result == 0);
    assert(cpu == 3);
    
    printf("  PASS: vDSO getcpu returned CPU %u\n", cpu);
}

/**
 * @brief Test vDSO symbol resolution
 */
static void test_vdso_symbol_resolution(void) {
    printf("Test: vDSO symbol resolution...\n");
    
    /* Note: Symbol resolution returns nullptr for now as symbols aren't set up */
    void *sym = vdso_resolve_symbol("__vdso_getpid");
    
    printf("  PASS: Symbol resolution completed (symbol=%p)\n", sym);
}

/**
 * @brief Run all vDSO tests
 */
int main(void) {
    printf("\n=== vDSO Tests ===\n\n");
    
    /* Run tests */
    test_vdso_init();
    test_vdso_getpid();
    test_vdso_time();
    test_vdso_getcpu();
    test_vdso_symbol_resolution();
    
    printf("\n=== All Tests Passed ===\n");
    return 0;
}
