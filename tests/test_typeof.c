
// Test suite for typeof and auto type inference

#include "c23_compat.h"
#include "test_framework.h"
#include <stddef.h>

// Test typeof with basic types
static bool test_typeof_basic_types(void) {
    int x = 42;
    typeof(x) y = x;
    ASSERT_EQ(y, 42, "typeof(int) should work");
    
    float f = 3.14f;
    typeof(f) g = f;
    ASSERT_TRUE(g > 3.13f && g < 3.15f, "typeof(float) should work");
    
    char c = 'A';
    typeof(c) d = c;
    ASSERT_EQ(d, 'A', "typeof(char) should work");
    
    return true;
}

// Test typeof with pointers
static bool test_typeof_pointers(void) {
    int value = 100;
    int* ptr = &value;
    typeof(ptr) ptr2 = ptr;
    
    ASSERT_NOT_NULL(ptr2, "typeof(int*) should work");
    ASSERT_EQ(*ptr2, 100, "dereferenced typeof pointer should equal 100");
    
    return true;
}

// Test typeof with arrays
static bool test_typeof_arrays(void) {
    int arr[5] = {1, 2, 3, 4, 5};
    typeof(arr[0]) first = arr[0];
    
    ASSERT_EQ(first, 1, "typeof(array element) should work");
    
    return true;
}

// Test typeof with structs
struct Point {
    int x;
    int y;
};

static bool test_typeof_structs(void) {
    struct Point p1 = {10, 20};
    typeof(p1) p2 = p1;
    
    ASSERT_EQ(p2.x, 10, "typeof(struct) x field should work");
    ASSERT_EQ(p2.y, 20, "typeof(struct) y field should work");
    
    return true;
}

// Test typeof in expressions
static bool test_typeof_expressions(void) {
    int a = 5;
    int b = 10;
    typeof(a + b) sum = a + b;
    
    ASSERT_EQ(sum, 15, "typeof(expression) should work");
    
    return true;
}

// Test typeof with const
static bool test_typeof_const(void) {
    const int x = 42;
    typeof(x) y = x;
    
    ASSERT_EQ(y, 42, "typeof(const int) should work");
    
    return true;
}

// Test auto keyword (C23)
static bool test_auto_keyword(void) {
    auto x = 42;
    ASSERT_EQ(x, 42, "auto should infer int type");
    
    auto f = 3.14f;
    ASSERT_TRUE(f > 3.13f && f < 3.15f, "auto should infer float type");
    
    auto ptr = &x;
    ASSERT_NOT_NULL(ptr, "auto should infer pointer type");
    ASSERT_EQ(*ptr, 42, "auto pointer dereference should work");
    
    return true;
}

// Test typeof with function pointers
typedef int (*BinaryOp)(int, int);

static int add(int a, int b) {
    return a + b;
}

static bool test_typeof_function_pointers(void) {
    BinaryOp op = add;
    typeof(op) op2 = op;
    
    ASSERT_NOT_NULL(op2, "typeof(function pointer) should work");
    ASSERT_EQ(op2(5, 3), 8, "typeof function pointer call should work");
    
    return true;
}

int main(void) {
    TEST_INIT();
    
    run_test("test_typeof_basic_types", test_typeof_basic_types);
    run_test("test_typeof_pointers", test_typeof_pointers);
    run_test("test_typeof_arrays", test_typeof_arrays);
    run_test("test_typeof_structs", test_typeof_structs);
    run_test("test_typeof_expressions", test_typeof_expressions);
    run_test("test_typeof_const", test_typeof_const);
    run_test("test_auto_keyword", test_auto_keyword);
    run_test("test_typeof_function_pointers", test_typeof_function_pointers);
    
    TEST_SUMMARY();
}

