
// Test suite for static_assert compile-time checks

#include "c23_compat.h"
#include "test_framework.h"
#include <stddef.h>
#include <stdint.h>

// Test basic static assertions
static_assert(sizeof(char) == 1, "char must be 1 byte");
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");
static_assert(sizeof(void*) >= 4, "pointer must be at least 4 bytes");

// Test struct size assertions
struct TestStruct {
    int a;
    char b;
    float c;
};

static_assert(sizeof(struct TestStruct) >= 9, "TestStruct must be at least 9 bytes");

// Test alignment assertions
static_assert(_Alignof(int) >= 1, "int alignment must be at least 1");
static_assert(_Alignof(double) >= 1, "double alignment must be at least 1");

// Test type size relationships
static_assert(sizeof(long) >= sizeof(int), "long must be at least as large as int");
static_assert(sizeof(long long) >= sizeof(long), "long long must be at least as large as long");

// Test pointer arithmetic
static_assert(sizeof(char*) == sizeof(void*), "all pointer types must have same size");
static_assert(sizeof(int*) == sizeof(void*), "all pointer types must have same size");

// Test integer types
static_assert(sizeof(int8_t) == 1, "int8_t must be 1 byte");
static_assert(sizeof(int16_t) == 2, "int16_t must be 2 bytes");
static_assert(sizeof(int32_t) == 4, "int32_t must be 4 bytes");
static_assert(sizeof(int64_t) == 8, "int64_t must be 8 bytes");

// Test boolean type
static_assert(sizeof(bool) == 1, "bool must be 1 byte");

// Test array sizes
static_assert(sizeof(int[10]) == 10 * sizeof(int), "array size must be element size * count");

// Test enum sizes
enum TestEnum {
    VALUE_A,
    VALUE_B,
    VALUE_C
};
static_assert(sizeof(enum TestEnum) >= sizeof(int), "enum must be at least int size");

// Runtime tests to verify static assertions compiled
static bool test_static_assertions_compiled(void) {
    ASSERT_TRUE(true, "all static assertions compiled successfully");
    return true;
}

static bool test_struct_sizes(void) {
    ASSERT_TRUE(sizeof(struct TestStruct) >= 9, "TestStruct size check");
    return true;
}

static bool test_integer_sizes(void) {
    ASSERT_EQ(sizeof(int8_t), 1, "int8_t size");
    ASSERT_EQ(sizeof(int16_t), 2, "int16_t size");
    ASSERT_EQ(sizeof(int32_t), 4, "int32_t size");
    ASSERT_EQ(sizeof(int64_t), 8, "int64_t size");
    return true;
}

static bool test_alignment_requirements(void) {
    ASSERT_TRUE(_Alignof(int) >= 1, "int alignment");
    ASSERT_TRUE(_Alignof(double) >= 1, "double alignment");
    return true;
}

static bool test_pointer_sizes(void) {
    ASSERT_EQ(sizeof(char*), sizeof(void*), "char* vs void* size");
    ASSERT_EQ(sizeof(int*), sizeof(void*), "int* vs void* size");
    ASSERT_EQ(sizeof(float*), sizeof(void*), "float* vs void* size");
    return true;
}

int main(void) {
    TEST_INIT();
    
    run_test("test_static_assertions_compiled", test_static_assertions_compiled);
    run_test("test_struct_sizes", test_struct_sizes);
    run_test("test_integer_sizes", test_integer_sizes);
    run_test("test_alignment_requirements", test_alignment_requirements);
    run_test("test_pointer_sizes", test_pointer_sizes);
    
    TEST_SUMMARY();
}

