
#include "../../framework/test_framework.h"
#include "../../../vm/value.h"
#include "../../../vm/object.h"

// Test value creation and type checking
static bool test_value_creation(void) {
    TEST_MEMORY_CHECKPOINT();
    
    // Test number values
    Value num_val = VALUE_NUMBER(42.5);
    TEST_ASSERT(IS_NUMBER(num_val), "Number value should be identified as number");
    TEST_ASSERT_EQ(42.5, AS_NUMBER(num_val), "Number value should retain its value");
    
    // Test boolean values
    Value bool_true = VALUE_BOOL(true);
    Value bool_false = VALUE_BOOL(false);
    TEST_ASSERT(IS_BOOL(bool_true), "Boolean value should be identified as boolean");
    TEST_ASSERT(IS_BOOL(bool_false), "Boolean value should be identified as boolean");
    TEST_ASSERT(AS_BOOL(bool_true), "True value should be true");
    TEST_ASSERT(!AS_BOOL(bool_false), "False value should be false");
    
    // Test nil value
    Value nil_val = VALUE_NIL;
    TEST_ASSERT(IS_NIL(nil_val), "Nil value should be identified as nil");
    
    TEST_MEMORY_VERIFY("Value creation should not leak memory");
    
    return true;
}

// Test value equality
static bool test_value_equality(void) {
    TEST_MEMORY_CHECKPOINT();
    
    // Test number equality
    Value num1 = VALUE_NUMBER(42.0);
    Value num2 = VALUE_NUMBER(42.0);
    Value num3 = VALUE_NUMBER(43.0);
    
    TEST_ASSERT(values_equal(num1, num2), "Equal numbers should be equal");
    TEST_ASSERT(!values_equal(num1, num3), "Different numbers should not be equal");
    
    // Test boolean equality
    Value bool1 = VALUE_BOOL(true);
    Value bool2 = VALUE_BOOL(true);
    Value bool3 = VALUE_BOOL(false);
    
    TEST_ASSERT(values_equal(bool1, bool2), "Equal booleans should be equal");
    TEST_ASSERT(!values_equal(bool1, bool3), "Different booleans should not be equal");
    
    // Test nil equality
    Value nil1 = VALUE_NIL;
    Value nil2 = VALUE_NIL;
    
    TEST_ASSERT(values_equal(nil1, nil2), "Nil values should be equal");
    
    // Test cross-type inequality
    TEST_ASSERT(!values_equal(num1, bool1), "Number and boolean should not be equal");
    TEST_ASSERT(!values_equal(num1, nil1), "Number and nil should not be equal");
    TEST_ASSERT(!values_equal(bool1, nil1), "Boolean and nil should not be equal");
    
    TEST_MEMORY_VERIFY("Value equality should not leak memory");
    
    return true;
}

// Test value arithmetic operations
static bool test_value_arithmetic(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Value a = VALUE_NUMBER(10.0);
    Value b = VALUE_NUMBER(3.0);
    
    // Test addition
    Value sum = value_add(a, b);
    TEST_ASSERT(IS_NUMBER(sum), "Addition result should be a number");
    TEST_ASSERT_EQ(13.0, AS_NUMBER(sum), "10 + 3 should equal 13");
    
    // Test subtraction
    Value diff = value_subtract(a, b);
    TEST_ASSERT(IS_NUMBER(diff), "Subtraction result should be a number");
    TEST_ASSERT_EQ(7.0, AS_NUMBER(diff), "10 - 3 should equal 7");
    
    // Test multiplication
    Value prod = value_multiply(a, b);
    TEST_ASSERT(IS_NUMBER(prod), "Multiplication result should be a number");
    TEST_ASSERT_EQ(30.0, AS_NUMBER(prod), "10 * 3 should equal 30");
    
    // Test division
    Value quot = value_divide(a, b);
    TEST_ASSERT(IS_NUMBER(quot), "Division result should be a number");
    TEST_ASSERT_EQ(10.0/3.0, AS_NUMBER(quot), "10 / 3 should equal 10/3");
    
    // Test division by zero
    Value zero = VALUE_NUMBER(0.0);
    Value div_by_zero = value_divide(a, zero);
    TEST_ASSERT(IS_NUMBER(div_by_zero), "Division by zero should return a number");
    TEST_ASSERT(isinf(AS_NUMBER(div_by_zero)), "Division by zero should return infinity");
    
    TEST_MEMORY_VERIFY("Value arithmetic should not leak memory");
    
    return true;
}

// Test value comparison operations
static bool test_value_comparison(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Value a = VALUE_NUMBER(10.0);
    Value b = VALUE_NUMBER(5.0);
    Value c = VALUE_NUMBER(10.0);
    
    // Test greater than
    TEST_ASSERT(value_greater(a, b), "10 should be greater than 5");
    TEST_ASSERT(!value_greater(b, a), "5 should not be greater than 10");
    TEST_ASSERT(!value_greater(a, c), "10 should not be greater than 10");
    
    // Test less than
    TEST_ASSERT(value_less(b, a), "5 should be less than 10");
    TEST_ASSERT(!value_less(a, b), "10 should not be less than 5");
    TEST_ASSERT(!value_less(a, c), "10 should not be less than 10");
    
    // Test greater than or equal
    TEST_ASSERT(value_greater_equal(a, b), "10 should be >= 5");
    TEST_ASSERT(value_greater_equal(a, c), "10 should be >= 10");
    TEST_ASSERT(!value_greater_equal(b, a), "5 should not be >= 10");
    
    // Test less than or equal
    TEST_ASSERT(value_less_equal(b, a), "5 should be <= 10");
    TEST_ASSERT(value_less_equal(a, c), "10 should be <= 10");
    TEST_ASSERT(!value_less_equal(a, b), "10 should not be <= 5");
    
    TEST_MEMORY_VERIFY("Value comparison should not leak memory");
    
    return true;
}

// Test value logical operations
static bool test_value_logical(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Value true_val = VALUE_BOOL(true);
    Value false_val = VALUE_BOOL(false);
    
    // Test logical AND
    Value and_tt = value_logical_and(true_val, true_val);
    Value and_tf = value_logical_and(true_val, false_val);
    Value and_ft = value_logical_and(false_val, true_val);
    Value and_ff = value_logical_and(false_val, false_val);
    
    TEST_ASSERT(IS_BOOL(and_tt) && AS_BOOL(and_tt), "true AND true should be true");
    TEST_ASSERT(IS_BOOL(and_tf) && !AS_BOOL(and_tf), "true AND false should be false");
    TEST_ASSERT(IS_BOOL(and_ft) && !AS_BOOL(and_ft), "false AND true should be false");
    TEST_ASSERT(IS_BOOL(and_ff) && !AS_BOOL(and_ff), "false AND false should be false");
    
    // Test logical OR
    Value or_tt = value_logical_or(true_val, true_val);
    Value or_tf = value_logical_or(true_val, false_val);
    Value or_ft = value_logical_or(false_val, true_val);
    Value or_ff = value_logical_or(false_val, false_val);
    
    TEST_ASSERT(IS_BOOL(or_tt) && AS_BOOL(or_tt), "true OR true should be true");
    TEST_ASSERT(IS_BOOL(or_tf) && AS_BOOL(or_tf), "true OR false should be true");
    TEST_ASSERT(IS_BOOL(or_ft) && AS_BOOL(or_ft), "false OR true should be true");
    TEST_ASSERT(IS_BOOL(or_ff) && !AS_BOOL(or_ff), "false OR false should be false");
    
    // Test logical NOT
    Value not_true = value_logical_not(true_val);
    Value not_false = value_logical_not(false_val);
    
    TEST_ASSERT(IS_BOOL(not_true) && !AS_BOOL(not_true), "NOT true should be false");
    TEST_ASSERT(IS_BOOL(not_false) && AS_BOOL(not_false), "NOT false should be true");
    
    TEST_MEMORY_VERIFY("Value logical operations should not leak memory");
    
    return true;
}

// Test value string operations
static bool test_value_string_operations(void) {
    TEST_MEMORY_CHECKPOINT();
    
    // Create string values
    Value str1 = VALUE_STRING("Hello");
    Value str2 = VALUE_STRING("World");
    Value str3 = VALUE_STRING("Hello");
    
    TEST_ASSERT(IS_STRING(str1), "String value should be identified as string");
    TEST_ASSERT(IS_STRING(str2), "String value should be identified as string");
    
    // Test string equality
    TEST_ASSERT(values_equal(str1, str3), "Equal strings should be equal");
    TEST_ASSERT(!values_equal(str1, str2), "Different strings should not be equal");
    
    // Test string concatenation
    Value concat = value_string_concat(str1, str2);
    TEST_ASSERT(IS_STRING(concat), "String concatenation should return string");
    TEST_ASSERT_STR_EQ("HelloWorld", AS_CSTRING(concat), "String concatenation should work correctly");
    
    // Test string length
    size_t len1 = value_string_length(str1);
    size_t len2 = value_string_length(str2);
    TEST_ASSERT_EQ(5, len1, "Length of 'Hello' should be 5");
    TEST_ASSERT_EQ(5, len2, "Length of 'World' should be 5");
    
    // Test string indexing
    Value char_at_0 = value_string_char_at(str1, 0);
    Value char_at_4 = value_string_char_at(str1, 4);
    TEST_ASSERT(IS_STRING(char_at_0), "Character at index should be string");
    TEST_ASSERT_STR_EQ("H", AS_CSTRING(char_at_0), "First character should be 'H'");
    TEST_ASSERT_STR_EQ("o", AS_CSTRING(char_at_4), "Last character should be 'o'");
    
    TEST_MEMORY_VERIFY("Value string operations should not leak memory");
    
    return true;
}

// Test value type conversion
static bool test_value_type_conversion(void) {
    TEST_MEMORY_CHECKPOINT();
    
    // Test number to string conversion
    Value num = VALUE_NUMBER(42.5);
    Value num_str = value_to_string(num);
    TEST_ASSERT(IS_STRING(num_str), "Number to string conversion should return string");
    TEST_ASSERT_STR_EQ("42.5", AS_CSTRING(num_str), "Number should convert to correct string");
    
    // Test boolean to string conversion
    Value bool_true = VALUE_BOOL(true);
    Value bool_false = VALUE_BOOL(false);
    Value true_str = value_to_string(bool_true);
    Value false_str = value_to_string(bool_false);
    TEST_ASSERT_STR_EQ("true", AS_CSTRING(true_str), "True should convert to 'true'");
    TEST_ASSERT_STR_EQ("false", AS_CSTRING(false_str), "False should convert to 'false'");
    
    // Test nil to string conversion
    Value nil_val = VALUE_NIL;
    Value nil_str = value_to_string(nil_val);
    TEST_ASSERT_STR_EQ("nil", AS_CSTRING(nil_str), "Nil should convert to 'nil'");
    
    // Test string to number conversion
    Value str_num = VALUE_STRING("123.45");
    Value converted_num = value_to_number(str_num);
    TEST_ASSERT(IS_NUMBER(converted_num), "String to number conversion should return number");
    TEST_ASSERT_EQ(123.45, AS_NUMBER(converted_num), "String should convert to correct number");
    
    // Test invalid string to number conversion
    Value invalid_str = VALUE_STRING("not_a_number");
    Value invalid_num = value_to_number(invalid_str);
    TEST_ASSERT(IS_NUMBER(invalid_num), "Invalid string conversion should still return number");
    TEST_ASSERT(isnan(AS_NUMBER(invalid_num)), "Invalid string should convert to NaN");
    
    TEST_MEMORY_VERIFY("Value type conversion should not leak memory");
    
    return true;
}

// Test value truthiness
static bool test_value_truthiness(void) {
    TEST_MEMORY_CHECKPOINT();
    
    // Test boolean truthiness
    Value true_val = VALUE_BOOL(true);
    Value false_val = VALUE_BOOL(false);
    TEST_ASSERT(value_is_truthy(true_val), "True should be truthy");
    TEST_ASSERT(!value_is_truthy(false_val), "False should be falsy");
    
    // Test nil truthiness
    Value nil_val = VALUE_NIL;
    TEST_ASSERT(!value_is_truthy(nil_val), "Nil should be falsy");
    
    // Test number truthiness
    Value zero = VALUE_NUMBER(0.0);
    Value non_zero = VALUE_NUMBER(42.0);
    Value negative = VALUE_NUMBER(-1.0);
    TEST_ASSERT(!value_is_truthy(zero), "Zero should be falsy");
    TEST_ASSERT(value_is_truthy(non_zero), "Non-zero should be truthy");
    TEST_ASSERT(value_is_truthy(negative), "Negative number should be truthy");
    
    // Test string truthiness
    Value empty_str = VALUE_STRING("");
    Value non_empty_str = VALUE_STRING("hello");
    TEST_ASSERT(!value_is_truthy(empty_str), "Empty string should be falsy");
    TEST_ASSERT(value_is_truthy(non_empty_str), "Non-empty string should be truthy");
    
    TEST_MEMORY_VERIFY("Value truthiness should not leak memory");
    
    return true;
}

// Test suite definition
static test_function_t value_system_tests[] = {
    test_value_creation,
    test_value_equality,
    test_value_arithmetic,
    test_value_comparison,
    test_value_logical,
    test_value_string_operations,
    test_value_type_conversion,
    test_value_truthiness
};

test_suite_t values_test_suite = {
    .name = "Value System Tests",
    .tests = value_system_tests,
    .test_count = sizeof(value_system_tests) / sizeof(value_system_tests[0])
};
