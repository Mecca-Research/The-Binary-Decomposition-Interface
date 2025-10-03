
// Test suite for nullptr migration validation

#include "c23_compat.h"
#include "test_framework.h"
#include <stddef.h>

// Test that nullptr is properly defined
static bool test_nullptr_defined(void) {
    void* ptr = nullptr;
    ASSERT_NULL(ptr, "nullptr should be NULL");
    return true;
}

// Test nullptr comparison
static bool test_nullptr_comparison(void) {
    void* ptr1 = nullptr;
    void* ptr2 = nullptr;
    ASSERT_TRUE(ptr1 == ptr2, "nullptr should equal nullptr");
    ASSERT_TRUE(ptr1 == nullptr, "pointer should equal nullptr");
    return true;
}

// Test nullptr assignment
static bool test_nullptr_assignment(void) {
    int* ptr = nullptr;
    ASSERT_NULL(ptr, "pointer should be nullptr after assignment");
    
    int value = 42;
    ptr = &value;
    ASSERT_NOT_NULL(ptr, "pointer should not be nullptr after valid assignment");
    ASSERT_EQ(*ptr, 42, "dereferenced pointer should equal 42");
    
    ptr = nullptr;
    ASSERT_NULL(ptr, "pointer should be nullptr after reassignment");
    return true;
}

// Test nullptr with different pointer types
static bool test_nullptr_types(void) {
    char* char_ptr = nullptr;
    int* int_ptr = nullptr;
    float* float_ptr = nullptr;
    void* void_ptr = nullptr;
    
    ASSERT_NULL(char_ptr, "char pointer should be nullptr");
    ASSERT_NULL(int_ptr, "int pointer should be nullptr");
    ASSERT_NULL(float_ptr, "float pointer should be nullptr");
    ASSERT_NULL(void_ptr, "void pointer should be nullptr");
    return true;
}

// Test nullptr in conditional expressions
static bool test_nullptr_conditionals(void) {
    void* ptr = nullptr;
    
    if (ptr) {
        ASSERT_TRUE(false, "nullptr should evaluate to false in if statement");
    }
    
    ASSERT_TRUE(!ptr, "!nullptr should be true");
    
    ptr = (void*)0x1234;
    ASSERT_TRUE(ptr != nullptr, "non-null pointer should not equal nullptr");
    return true;
}

int main(void) {
    TEST_INIT();
    
    run_test("test_nullptr_defined", test_nullptr_defined);
    run_test("test_nullptr_comparison", test_nullptr_comparison);
    run_test("test_nullptr_assignment", test_nullptr_assignment);
    run_test("test_nullptr_types", test_nullptr_types);
    run_test("test_nullptr_conditionals", test_nullptr_conditionals);
    
    TEST_SUMMARY();
}

