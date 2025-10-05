
#include "../../framework/test_framework.h"
#include "../../../vm/vm.h"
#include "../../../vm/error.h"

// Test error creation and management
static bool test_error_creation(void) {
    TEST_MEMORY_CHECKPOINT();
    
    // Test basic error creation
    Error* error = error_create(ERROR_RUNTIME, "Test error message", 42);
    TEST_ASSERT_NOT_NULL(error, "Error creation should succeed");
    
    TEST_ASSERT_EQ(ERROR_RUNTIME, error_get_type(error), "Error type should match");
    TEST_ASSERT_STR_EQ("Test error message", error_get_message(error), "Error message should match");
    TEST_ASSERT_EQ(42, error_get_line(error), "Error line should match");
    
    error_destroy(error);
    TEST_MEMORY_VERIFY("Error creation should not leak memory");
    
    return true;
}

// Test error stack trace
static bool test_error_stack_trace(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Error* error = error_create(ERROR_RUNTIME, "Stack trace test", 10);
    TEST_ASSERT_NOT_NULL(error, "Error creation should succeed");
    
    // Add stack frames
    error_add_stack_frame(error, "main", "test.bdi", 10);
    error_add_stack_frame(error, "foo", "test.bdi", 5);
    error_add_stack_frame(error, "bar", "test.bdi", 2);
    
    TEST_ASSERT_EQ(3, error_get_stack_depth(error), "Stack should have 3 frames");
    
    // Check stack frames
    StackFrame* frame0 = error_get_stack_frame(error, 0);
    StackFrame* frame1 = error_get_stack_frame(error, 1);
    StackFrame* frame2 = error_get_stack_frame(error, 2);
    
    TEST_ASSERT_STR_EQ("main", frame0->function_name, "First frame should be 'main'");
    TEST_ASSERT_STR_EQ("foo", frame1->function_name, "Second frame should be 'foo'");
    TEST_ASSERT_STR_EQ("bar", frame2->function_name, "Third frame should be 'bar'");
    
    // Test stack trace formatting
    char* stack_trace = error_format_stack_trace(error);
    TEST_ASSERT_NOT_NULL(stack_trace, "Stack trace formatting should succeed");
    TEST_ASSERT(strstr(stack_trace, "main") != NULL, "Stack trace should contain 'main'");
    TEST_ASSERT(strstr(stack_trace, "foo") != NULL, "Stack trace should contain 'foo'");
    TEST_ASSERT(strstr(stack_trace, "bar") != NULL, "Stack trace should contain 'bar'");
    
    free(stack_trace);
    error_destroy(error);
    TEST_MEMORY_VERIFY("Error stack trace should not leak memory");
    
    return true;
}

// Test error propagation
static bool test_error_propagation(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Test error propagation through VM
    TEST_ASSERT(!vm_has_error(vm), "VM should start without errors");
    
    // Simulate an error in nested function calls
    vm_push_call_frame(vm, 100, 2);
    vm_push_call_frame(vm, 200, 1);
    
    // Create and propagate error
    Error* error = error_create(ERROR_RUNTIME, "Nested error", 25);
    error_add_stack_frame(error, "inner", "test.bdi", 25);
    error_add_stack_frame(error, "outer", "test.bdi", 15);
    
    vm_set_error_object(vm, error);
    TEST_ASSERT(vm_has_error(vm), "VM should have error after setting");
    
    // Test error retrieval
    Error* retrieved_error = vm_get_error_object(vm);
    TEST_ASSERT_NOT_NULL(retrieved_error, "Should be able to retrieve error object");
    TEST_ASSERT_EQ(ERROR_RUNTIME, error_get_type(retrieved_error), "Error type should be preserved");
    TEST_ASSERT_STR_EQ("Nested error", error_get_message(retrieved_error), "Error message should be preserved");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Error propagation should not leak memory");
    
    return true;
}

// Test different error types
static bool test_error_types(void) {
    TEST_MEMORY_CHECKPOINT();
    
    // Test syntax error
    Error* syntax_error = error_create(ERROR_SYNTAX, "Unexpected token", 5);
    TEST_ASSERT_EQ(ERROR_SYNTAX, error_get_type(syntax_error), "Syntax error type should be correct");
    
    // Test runtime error
    Error* runtime_error = error_create(ERROR_RUNTIME, "Division by zero", 10);
    TEST_ASSERT_EQ(ERROR_RUNTIME, error_get_type(runtime_error), "Runtime error type should be correct");
    
    // Test type error
    Error* type_error = error_create(ERROR_TYPE, "Cannot add string and number", 15);
    TEST_ASSERT_EQ(ERROR_TYPE, error_get_type(type_error), "Type error type should be correct");
    
    // Test memory error
    Error* memory_error = error_create(ERROR_MEMORY, "Out of memory", 20);
    TEST_ASSERT_EQ(ERROR_MEMORY, error_get_type(memory_error), "Memory error type should be correct");
    
    // Test error type names
    TEST_ASSERT_STR_EQ("SyntaxError", error_type_name(ERROR_SYNTAX), "Syntax error name should be correct");
    TEST_ASSERT_STR_EQ("RuntimeError", error_type_name(ERROR_RUNTIME), "Runtime error name should be correct");
    TEST_ASSERT_STR_EQ("TypeError", error_type_name(ERROR_TYPE), "Type error name should be correct");
    TEST_ASSERT_STR_EQ("MemoryError", error_type_name(ERROR_MEMORY), "Memory error name should be correct");
    
    error_destroy(syntax_error);
    error_destroy(runtime_error);
    error_destroy(type_error);
    error_destroy(memory_error);
    TEST_MEMORY_VERIFY("Error types should not leak memory");
    
    return true;
}

// Test error recovery mechanisms
static bool test_error_recovery(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Set up error state
    vm_set_error(vm, "Test error for recovery");
    TEST_ASSERT(vm_has_error(vm), "VM should have error");
    
    // Test error clearing
    vm_clear_error(vm);
    TEST_ASSERT(!vm_has_error(vm), "VM should not have error after clearing");
    
    // Test that VM can continue normal operation after error recovery
    vm_push_int(vm, 42);
    TEST_ASSERT_EQ(1, vm_stack_size(vm), "VM should function normally after error recovery");
    TEST_ASSERT_EQ(42, vm_pop_int(vm), "VM should return correct values after error recovery");
    
    // Test exception handling mechanism
    bool caught = false;
    vm_try_begin(vm);
    
    // Simulate operation that might fail
    vm_set_error(vm, "Simulated error");
    
    if (vm_try_catch(vm)) {
        caught = true;
        const char* error_msg = vm_get_error(vm);
        TEST_ASSERT_STR_EQ("Simulated error", error_msg, "Caught error should have correct message");
        vm_clear_error(vm);
    }
    
    vm_try_end(vm);
    TEST_ASSERT(caught, "Error should have been caught by try-catch mechanism");
    TEST_ASSERT(!vm_has_error(vm), "Error should be cleared after handling");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Error recovery should not leak memory");
    
    return true;
}

// Test error formatting and display
static bool test_error_formatting(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Error* error = error_create(ERROR_RUNTIME, "Test formatting error", 42);
    error_add_stack_frame(error, "test_function", "test_file.bdi", 42);
    error_add_stack_frame(error, "main", "test_file.bdi", 10);
    
    // Test basic error formatting
    char* basic_format = error_format_basic(error);
    TEST_ASSERT_NOT_NULL(basic_format, "Basic error formatting should succeed");
    TEST_ASSERT(strstr(basic_format, "RuntimeError") != NULL, "Formatted error should contain error type");
    TEST_ASSERT(strstr(basic_format, "Test formatting error") != NULL, "Formatted error should contain message");
    TEST_ASSERT(strstr(basic_format, "42") != NULL, "Formatted error should contain line number");
    
    // Test detailed error formatting
    char* detailed_format = error_format_detailed(error);
    TEST_ASSERT_NOT_NULL(detailed_format, "Detailed error formatting should succeed");
    TEST_ASSERT(strstr(detailed_format, "test_function") != NULL, "Detailed format should contain function names");
    TEST_ASSERT(strstr(detailed_format, "test_file.bdi") != NULL, "Detailed format should contain file names");
    
    // Test JSON error formatting
    char* json_format = error_format_json(error);
    TEST_ASSERT_NOT_NULL(json_format, "JSON error formatting should succeed");
    TEST_ASSERT(strstr(json_format, "\"type\"") != NULL, "JSON format should contain type field");
    TEST_ASSERT(strstr(json_format, "\"message\"") != NULL, "JSON format should contain message field");
    TEST_ASSERT(strstr(json_format, "\"stackTrace\"") != NULL, "JSON format should contain stack trace field");
    
    free(basic_format);
    free(detailed_format);
    free(json_format);
    error_destroy(error);
    TEST_MEMORY_VERIFY("Error formatting should not leak memory");
    
    return true;
}

// Test error context and source information
static bool test_error_context(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Error* error = error_create(ERROR_SYNTAX, "Unexpected token ';'", 15);
    
    // Add source context
    const char* source_line = "var x = 42;; // Extra semicolon";
    error_set_source_context(error, source_line, 11); // Point to extra semicolon
    
    // Test context retrieval
    const char* context = error_get_source_context(error);
    TEST_ASSERT_NOT_NULL(context, "Source context should be available");
    TEST_ASSERT_STR_EQ(source_line, context, "Source context should match original");
    
    TEST_ASSERT_EQ(11, error_get_column(error), "Error column should be correct");
    
    // Test context formatting with highlighting
    char* highlighted = error_format_with_context(error);
    TEST_ASSERT_NOT_NULL(highlighted, "Context formatting should succeed");
    TEST_ASSERT(strstr(highlighted, source_line) != NULL, "Formatted error should contain source line");
    TEST_ASSERT(strstr(highlighted, "^") != NULL, "Formatted error should contain position indicator");
    
    free(highlighted);
    error_destroy(error);
    TEST_MEMORY_VERIFY("Error context should not leak memory");
    
    return true;
}

// Test error chaining
static bool test_error_chaining(void) {
    TEST_MEMORY_CHECKPOINT();
    
    // Create root cause error
    Error* root_error = error_create(ERROR_MEMORY, "Out of memory", 5);
    
    // Create higher-level error that wraps the root cause
    Error* wrapper_error = error_create(ERROR_RUNTIME, "Failed to allocate object", 10);
    error_set_cause(wrapper_error, root_error);
    
    // Test error chaining
    TEST_ASSERT(error_has_cause(wrapper_error), "Wrapper error should have a cause");
    Error* cause = error_get_cause(wrapper_error);
    TEST_ASSERT_NOT_NULL(cause, "Should be able to retrieve cause");
    TEST_ASSERT_EQ(ERROR_MEMORY, error_get_type(cause), "Cause should have correct type");
    TEST_ASSERT_STR_EQ("Out of memory", error_get_message(cause), "Cause should have correct message");
    
    // Test chained error formatting
    char* chained_format = error_format_chained(wrapper_error);
    TEST_ASSERT_NOT_NULL(chained_format, "Chained error formatting should succeed");
    TEST_ASSERT(strstr(chained_format, "Failed to allocate object") != NULL, "Should contain wrapper message");
    TEST_ASSERT(strstr(chained_format, "Out of memory") != NULL, "Should contain root cause message");
    TEST_ASSERT(strstr(chained_format, "Caused by:") != NULL, "Should contain causation indicator");
    
    free(chained_format);
    error_destroy(wrapper_error); // This should also destroy the root error
    TEST_MEMORY_VERIFY("Error chaining should not leak memory");
    
    return true;
}

// Test suite definition
static test_function_t error_handling_tests[] = {
    test_error_creation,
    test_error_stack_trace,
    test_error_propagation,
    test_error_types,
    test_error_recovery,
    test_error_formatting,
    test_error_context,
    test_error_chaining
};

test_suite_t errors_test_suite = {
    .name = "Error Handling Tests",
    .tests = error_handling_tests,
    .test_count = sizeof(error_handling_tests) / sizeof(error_handling_tests[0])
};
