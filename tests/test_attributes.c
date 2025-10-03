
// Test suite for C23 attribute annotations

#include "c23_compat.h"
#include "test_framework.h"
#include <stddef.h>

// Test [[nodiscard]] attribute
[[nodiscard]] static int get_error_code(void) {
    return -1;
}

[[nodiscard]] static void* allocate_memory(size_t size) {
    return malloc(size);
}

static bool test_nodiscard_attribute(void) {
    // These should generate warnings if result is not used
    int error = get_error_code();
    ASSERT_EQ(error, -1, "error code should be -1");
    
    void* ptr = allocate_memory(100);
    ASSERT_NOT_NULL(ptr, "allocated memory should not be nullptr");
    free(ptr);
    return true;
}

// Test [[maybe_unused]] attribute
static bool test_maybe_unused_attribute(void) {
    [[maybe_unused]] int unused_var = 42;
    [[maybe_unused]] const char* unused_str = "test";
    
    // These variables may not be used, but shouldn't generate warnings
    return true;
}

// Test [[noreturn]] attribute
[[noreturn]] static void fatal_error(const char* msg) {
    fprintf(stderr, "Fatal error: %s\n", msg);
    exit(EXIT_FAILURE);
}

static bool test_noreturn_attribute(void) {
    // We can't actually test noreturn without exiting
    // This test just verifies the attribute compiles
    ASSERT_TRUE(true, "noreturn attribute compiles");
    return true;
}

// Test [[fallthrough]] attribute
static int test_fallthrough_switch(int value) {
    int result = 0;
    switch (value) {
        case 1:
            result += 10;
            [[fallthrough]];
        case 2:
            result += 20;
            [[fallthrough]];
        case 3:
            result += 30;
            break;
        default:
            result = -1;
    }
    return result;
}

static bool test_fallthrough_attribute(void) {
    ASSERT_EQ(test_fallthrough_switch(1), 60, "case 1 should fall through to 2 and 3");
    ASSERT_EQ(test_fallthrough_switch(2), 50, "case 2 should fall through to 3");
    ASSERT_EQ(test_fallthrough_switch(3), 30, "case 3 should not fall through");
    ASSERT_EQ(test_fallthrough_switch(5), -1, "default case should return -1");
    return true;
}

int main(void) {
    TEST_INIT();
    
    run_test("test_nodiscard_attribute", test_nodiscard_attribute);
    run_test("test_maybe_unused_attribute", test_maybe_unused_attribute);
    run_test("test_noreturn_attribute", test_noreturn_attribute);
    run_test("test_fallthrough_attribute", test_fallthrough_attribute);
    
    TEST_SUMMARY();
}

