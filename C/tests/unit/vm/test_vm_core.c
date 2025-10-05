
#include "../../framework/test_framework.h"
#include "../../../vm/vm.h"
#include "../../../vm/bci_vm.h"
#include "../../../vm/bci_chunk.h"

// Test VM initialization and cleanup
static bool test_vm_init_cleanup(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Test initial state
    TEST_ASSERT_EQ(0, vm_stack_size(vm), "Initial stack should be empty");
    TEST_ASSERT_NOT_NULL(vm_get_globals(vm), "Globals should be initialized");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("VM init/cleanup should not leak memory");
    
    return true;
}

// Test VM stack operations
static bool test_vm_stack_operations(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Test push operations
    vm_push_int(vm, 42);
    TEST_ASSERT_EQ(1, vm_stack_size(vm), "Stack size should be 1 after push");
    
    vm_push_double(vm, 3.14);
    TEST_ASSERT_EQ(2, vm_stack_size(vm), "Stack size should be 2 after second push");
    
    // Test peek operations
    double d = vm_peek_double(vm, 0);
    TEST_ASSERT_EQ(3.14, d, "Peek should return last pushed value");
    
    int i = vm_peek_int(vm, 1);
    TEST_ASSERT_EQ(42, i, "Peek should return first pushed value");
    
    // Test pop operations
    double popped_d = vm_pop_double(vm);
    TEST_ASSERT_EQ(3.14, popped_d, "Pop should return last pushed value");
    TEST_ASSERT_EQ(1, vm_stack_size(vm), "Stack size should decrease after pop");
    
    int popped_i = vm_pop_int(vm);
    TEST_ASSERT_EQ(42, popped_i, "Pop should return remaining value");
    TEST_ASSERT_EQ(0, vm_stack_size(vm), "Stack should be empty after all pops");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("VM stack operations should not leak memory");
    
    return true;
}

// Test VM stack overflow protection
static bool test_vm_stack_overflow(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Push many values to test overflow protection
    bool overflow_detected = false;
    for (int i = 0; i < 10000; i++) {
        if (!vm_push_int(vm, i)) {
            overflow_detected = true;
            break;
        }
    }
    
    TEST_ASSERT(overflow_detected, "Stack overflow should be detected");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("VM stack overflow test should not leak memory");
    
    return true;
}

// Test VM global variable operations
static bool test_vm_globals(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Test setting and getting global variables
    vm_set_global_int(vm, "test_var", 123);
    int value = vm_get_global_int(vm, "test_var");
    TEST_ASSERT_EQ(123, value, "Global variable should retain its value");
    
    // Test overwriting global variables
    vm_set_global_int(vm, "test_var", 456);
    value = vm_get_global_int(vm, "test_var");
    TEST_ASSERT_EQ(456, value, "Global variable should be updated");
    
    // Test non-existent global variables
    bool exists = vm_has_global(vm, "nonexistent");
    TEST_ASSERT(!exists, "Non-existent global should return false");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("VM globals test should not leak memory");
    
    return true;
}

// Test VM error handling
static bool test_vm_error_handling(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Test error state management
    TEST_ASSERT(!vm_has_error(vm), "VM should start without errors");
    
    vm_set_error(vm, "Test error message");
    TEST_ASSERT(vm_has_error(vm), "VM should have error after setting");
    
    const char* error_msg = vm_get_error(vm);
    TEST_ASSERT_STR_EQ("Test error message", error_msg, "Error message should match");
    
    vm_clear_error(vm);
    TEST_ASSERT(!vm_has_error(vm), "VM should not have error after clearing");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("VM error handling should not leak memory");
    
    return true;
}

// Test VM state serialization
static bool test_vm_state_serialization(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Set up some state
    vm_push_int(vm, 42);
    vm_push_double(vm, 3.14);
    vm_set_global_int(vm, "test", 123);
    
    // Serialize state
    size_t state_size;
    void* state_data = vm_serialize_state(vm, &state_size);
    TEST_ASSERT_NOT_NULL(state_data, "State serialization should succeed");
    TEST_ASSERT(state_size > 0, "Serialized state should have non-zero size");
    
    // Create new VM and deserialize
    VM* vm2 = vm_create();
    bool success = vm_deserialize_state(vm2, state_data, state_size);
    TEST_ASSERT(success, "State deserialization should succeed");
    
    // Verify state was restored
    TEST_ASSERT_EQ(2, vm_stack_size(vm2), "Stack size should be restored");
    TEST_ASSERT_EQ(3.14, vm_peek_double(vm2, 0), "Stack values should be restored");
    TEST_ASSERT_EQ(42, vm_peek_int(vm2, 1), "Stack values should be restored");
    TEST_ASSERT_EQ(123, vm_get_global_int(vm2, "test"), "Global values should be restored");
    
    free(state_data);
    vm_destroy(vm);
    vm_destroy(vm2);
    TEST_MEMORY_VERIFY("VM state serialization should not leak memory");
    
    return true;
}

// Test VM instruction pointer management
static bool test_vm_instruction_pointer(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Test initial IP
    TEST_ASSERT_EQ(0, vm_get_ip(vm), "Initial IP should be 0");
    
    // Test IP manipulation
    vm_set_ip(vm, 100);
    TEST_ASSERT_EQ(100, vm_get_ip(vm), "IP should be set correctly");
    
    vm_advance_ip(vm, 5);
    TEST_ASSERT_EQ(105, vm_get_ip(vm), "IP should advance correctly");
    
    vm_jump_ip(vm, 50);
    TEST_ASSERT_EQ(50, vm_get_ip(vm), "IP should jump correctly");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("VM IP management should not leak memory");
    
    return true;
}

// Test VM call stack operations
static bool test_vm_call_stack(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Test call stack operations
    TEST_ASSERT_EQ(0, vm_call_stack_depth(vm), "Initial call stack should be empty");
    
    // Push call frame
    vm_push_call_frame(vm, 100, 5);
    TEST_ASSERT_EQ(1, vm_call_stack_depth(vm), "Call stack depth should increase");
    
    // Push another call frame
    vm_push_call_frame(vm, 200, 3);
    TEST_ASSERT_EQ(2, vm_call_stack_depth(vm), "Call stack depth should increase");
    
    // Pop call frame
    CallFrame frame = vm_pop_call_frame(vm);
    TEST_ASSERT_EQ(200, frame.return_address, "Popped frame should have correct return address");
    TEST_ASSERT_EQ(3, frame.local_count, "Popped frame should have correct local count");
    TEST_ASSERT_EQ(1, vm_call_stack_depth(vm), "Call stack depth should decrease");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("VM call stack should not leak memory");
    
    return true;
}

// Test VM performance monitoring
static bool test_vm_performance_monitoring(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Test performance counters
    TEST_ASSERT_EQ(0, vm_get_instruction_count(vm), "Initial instruction count should be 0");
    TEST_ASSERT_EQ(0, vm_get_execution_time(vm), "Initial execution time should be 0");
    
    // Simulate some execution
    vm_increment_instruction_count(vm);
    vm_increment_instruction_count(vm);
    TEST_ASSERT_EQ(2, vm_get_instruction_count(vm), "Instruction count should increment");
    
    // Test timing
    vm_start_timing(vm);
    usleep(1000); // Sleep for 1ms
    vm_stop_timing(vm);
    
    uint64_t exec_time = vm_get_execution_time(vm);
    TEST_ASSERT(exec_time > 0, "Execution time should be recorded");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("VM performance monitoring should not leak memory");
    
    return true;
}

// Test VM memory management integration
static bool test_vm_memory_management(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Test memory allocation tracking
    size_t initial_memory = vm_get_memory_usage(vm);
    
    // Allocate some objects
    for (int i = 0; i < 100; i++) {
        vm_allocate_object(vm, sizeof(int) * 10);
    }
    
    size_t after_alloc = vm_get_memory_usage(vm);
    TEST_ASSERT(after_alloc > initial_memory, "Memory usage should increase after allocation");
    
    // Trigger garbage collection
    vm_collect_garbage(vm);
    
    size_t after_gc = vm_get_memory_usage(vm);
    TEST_ASSERT(after_gc <= after_alloc, "Memory usage should not increase after GC");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("VM memory management should not leak memory");
    
    return true;
}

// Test suite definition
static test_function_t vm_core_tests[] = {
    test_vm_init_cleanup,
    test_vm_stack_operations,
    test_vm_stack_overflow,
    test_vm_globals,
    test_vm_error_handling,
    test_vm_state_serialization,
    test_vm_instruction_pointer,
    test_vm_call_stack,
    test_vm_performance_monitoring,
    test_vm_memory_management
};

test_suite_t vm_test_suite = {
    .name = "VM Core Tests",
    .tests = vm_core_tests,
    .test_count = sizeof(vm_core_tests) / sizeof(vm_core_tests[0])
};
