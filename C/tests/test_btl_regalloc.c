// BTL Register Allocator Tests
#include "../btl/btl_regalloc.h"
#include <stdio.h>
#include <assert.h>

static int tests_passed = 0;

#define TEST(name) printf("Testing %s... ", name); fflush(stdout);
#define PASS() printf("PASS\n"); tests_passed++;

void test_allocator_creation(void) {
    TEST("allocator creation");
    
    BTL_RegAllocator *alloc = btl_regalloc_create(16, BTL_REG_GENERAL);
    assert(alloc != NULL);
    assert(btl_regalloc_get_free_count(alloc) == 16);
    
    btl_regalloc_destroy(alloc);
    PASS();
}

void test_register_acquire_release(void) {
    TEST("register acquire/release");
    
    BTL_RegAllocator *alloc = btl_regalloc_create(8, BTL_REG_GENERAL);
    
    int reg1 = btl_regalloc_acquire(alloc, 1, 0, 10);
    assert(reg1 >= 0);
    assert(btl_regalloc_get_allocated_count(alloc) == 1);
    
    int reg2 = btl_regalloc_acquire(alloc, 2, 5, 15);
    assert(reg2 >= 0 && reg2 != reg1);
    assert(btl_regalloc_get_allocated_count(alloc) == 2);
    
    btl_regalloc_release(alloc, reg1);
    assert(btl_regalloc_get_allocated_count(alloc) == 1);
    assert(btl_regalloc_is_available(alloc, reg1));
    
    btl_regalloc_destroy(alloc);
    PASS();
}

void test_linear_scan(void) {
    TEST("linear scan allocation");
    
    BTL_RegAllocator *alloc = btl_regalloc_create(4, BTL_REG_GENERAL);
    
    // Add live intervals
    btl_regalloc_add_interval(alloc, 1, 0, 10);
    btl_regalloc_add_interval(alloc, 2, 5, 15);
    btl_regalloc_add_interval(alloc, 3, 12, 20);
    btl_regalloc_add_interval(alloc, 4, 8, 18);
    
    bool success = btl_regalloc_linear_scan(alloc);
    assert(success);
    
    btl_regalloc_destroy(alloc);
    PASS();
}

void test_spilling(void) {
    TEST("register spilling");
    
    BTL_RegAllocator *alloc = btl_regalloc_create(2, BTL_REG_GENERAL);
    
    // Add more intervals than registers
    btl_regalloc_add_interval(alloc, 1, 0, 10);
    btl_regalloc_add_interval(alloc, 2, 0, 10);
    btl_regalloc_add_interval(alloc, 3, 0, 10);
    
    btl_regalloc_linear_scan(alloc);
    
    assert(btl_regalloc_needs_spill(alloc));
    assert(btl_regalloc_get_spill_count(alloc) > 0);
    
    btl_regalloc_destroy(alloc);
    PASS();
}

int main(void) {
    printf("=== BTL Register Allocator Tests ===\n\n");
    
    test_allocator_creation();
    test_register_acquire_release();
    test_linear_scan();
    test_spilling();
    
    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    return 0;
}
