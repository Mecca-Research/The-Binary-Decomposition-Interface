
#include "../../framework/test_framework.h"
#include "../../../vm/vm.h"
#include "../../../vm/bci_vm.h"
#include "../../../vm/bci_chunk.h"

// Test VM initialization and cleanup
static bool test_vm_init_cleanup(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM vm;
    vm_init(&vm);
    
    // Test initial state
    TEST_ASSERT_NOT_NULL(vm.stack, "VM stack should be initialized");
    TEST_ASSERT_EQ(vm.stack, vm.stack_top, "Initial stack should be empty");
    TEST_ASSERT_NULL(vm.chunk, "Initial chunk should be NULL");
    TEST_ASSERT_NULL(vm.ip, "Initial IP should be NULL");
    
    vm_free(&vm);
    TEST_MEMORY_VERIFY("VM init/cleanup should not leak memory");
    
    return true;
}

// Test VM stack operations using available API
static bool test_vm_stack_operations(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM vm;
    vm_init(&vm);
    
    // Test push operations using available API
    vm_stack_push(&vm, 42.0);
    TEST_ASSERT_EQ(1, vm.stack_top - vm.stack, "Stack size should be 1 after push");
    
    vm_stack_push(&vm, 3.14);
    TEST_ASSERT_EQ(2, vm.stack_top - vm.stack, "Stack size should be 2 after second push");
    
    // Test pop operations using available API
    double popped_d = vm_stack_pop(&vm);
    TEST_ASSERT_EQ(3.14, popped_d, "Pop should return last pushed value");
    TEST_ASSERT_EQ(1, vm.stack_top - vm.stack, "Stack size should decrease after pop");
    
    double popped_i = vm_stack_pop(&vm);
    TEST_ASSERT_EQ(42.0, popped_i, "Pop should return remaining value");
    TEST_ASSERT_EQ(0, vm.stack_top - vm.stack, "Stack should be empty after all pops");
    
    vm_free(&vm);
    TEST_MEMORY_VERIFY("VM stack operations should not leak memory");
    
    return true;
}

// Test VM stack overflow protection
static bool test_vm_stack_overflow(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM vm;
    vm_init(&vm);
    
    // Push values up to stack limit
    for (int i = 0; i < STACK_MAX; i++) {
        vm_stack_push(&vm, (double)i);
    }
    
    // Verify stack is at maximum capacity
    TEST_ASSERT_EQ(STACK_MAX, vm.stack_top - vm.stack, "Stack should be at maximum capacity");
    
    // Note: The current VM implementation doesn't have overflow protection,
    // but we can test that we've reached the expected limit
    
    vm_free(&vm);
    TEST_MEMORY_VERIFY("VM stack overflow test should not leak memory");
    
    return true;
}

// Test VM reset functionality
static bool test_vm_reset(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM vm;
    vm_init(&vm);
    
    // Add some state to the VM
    vm_stack_push(&vm, 42.0);
    vm_stack_push(&vm, 3.14);
    
    // Reset the VM
    vm_reset(&vm);
    
    // Verify VM is reset
    TEST_ASSERT_EQ(vm.stack, vm.stack_top, "Stack should be empty after reset");
    TEST_ASSERT_NULL(vm.chunk, "Chunk should be NULL after reset");
    TEST_ASSERT_NULL(vm.ip, "IP should be NULL after reset");
    
    vm_free(&vm);
    TEST_MEMORY_VERIFY("VM reset should not leak memory");
    
    return true;
}

// Test VM interpretation with simple bytecode
static bool test_vm_interpretation(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Create simple bytecode: push constant 42, return
    int constant_index = chunk_add_constant(&chunk, 42.0);
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, constant_index, 1);
    chunk_write(&chunk, OP_RETURN, 1);
    
    // Test interpretation
    InterpretResult result = vm_interpret(&vm, &chunk);
    TEST_ASSERT_EQ(INTERPRET_OK, result, "Simple bytecode should execute successfully");
    
    chunk_free(&chunk);
    vm_free(&vm);
    TEST_MEMORY_VERIFY("VM interpretation should not leak memory");
    
    return true;
}

// Test VM interpretation with result capture
static bool test_vm_interpretation_with_result(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Create bytecode: push constant 3.14, return
    int constant_index = chunk_add_constant(&chunk, 3.14);
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, constant_index, 1);
    chunk_write(&chunk, OP_RETURN, 1);
    
    // Test interpretation with result
    BciVmResult result = vm_interpret_with_result(&vm, &chunk);
    TEST_ASSERT_EQ(INTERPRET_OK, result.status, "Bytecode should execute successfully");
    TEST_ASSERT_EQ(3.14, result.result_value, "Result value should be captured correctly");
    
    chunk_free(&chunk);
    vm_free(&vm);
    TEST_MEMORY_VERIFY("VM interpretation with result should not leak memory");
    
    return true;
}

// Test VM arithmetic operations
static bool test_vm_arithmetic(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Create bytecode: 5 + 3 = 8
    int const1 = chunk_add_constant(&chunk, 5.0);
    int const2 = chunk_add_constant(&chunk, 3.0);
    
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, const1, 1);
    chunk_write(&chunk, OP_CONSTANT, 2);
    chunk_write(&chunk, const2, 2);
    chunk_write(&chunk, OP_ADD, 3);
    chunk_write(&chunk, OP_RETURN, 3);
    
    // Test arithmetic execution
    BciVmResult result = vm_interpret_with_result(&vm, &chunk);
    TEST_ASSERT_EQ(INTERPRET_OK, result.status, "Arithmetic should execute successfully");
    TEST_ASSERT_EQ(8.0, result.result_value, "5 + 3 should equal 8");
    
    chunk_free(&chunk);
    vm_free(&vm);
    TEST_MEMORY_VERIFY("VM arithmetic should not leak memory");
    
    return true;
}

// Test VM negation operation
static bool test_vm_negation(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Create bytecode: -42
    int constant_index = chunk_add_constant(&chunk, 42.0);
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, constant_index, 1);
    chunk_write(&chunk, OP_NEGATE, 1);
    chunk_write(&chunk, OP_RETURN, 1);
    
    // Test negation
    BciVmResult result = vm_interpret_with_result(&vm, &chunk);
    TEST_ASSERT_EQ(INTERPRET_OK, result.status, "Negation should execute successfully");
    TEST_ASSERT_EQ(-42.0, result.result_value, "Negation of 42 should be -42");
    
    chunk_free(&chunk);
    vm_free(&vm);
    TEST_MEMORY_VERIFY("VM negation should not leak memory");
    
    return true;
}

// Test Enhanced VM creation and destruction
static bool test_enhanced_vm_lifecycle(void) {
    TEST_MEMORY_CHECKPOINT();
    
    EnhancedVM* evm = enhanced_vm_create(1024 * 1024); // 1MB heap
    TEST_ASSERT_NOT_NULL(evm, "Enhanced VM creation should succeed");
    TEST_ASSERT_NOT_NULL(evm->base_vm, "Base VM should be initialized");
    
    enhanced_vm_destroy(evm);
    TEST_MEMORY_VERIFY("Enhanced VM lifecycle should not leak memory");
    
    return true;
}

// Test Enhanced VM configuration
static bool test_enhanced_vm_configuration(void) {
    TEST_MEMORY_CHECKPOINT();
    
    EnhancedVM* evm = enhanced_vm_create(1024 * 1024);
    TEST_ASSERT_NOT_NULL(evm, "Enhanced VM creation should succeed");
    
    // Test configuration functions
    enhanced_vm_enable_jit(evm, true);
    TEST_ASSERT(evm->enable_jit, "JIT should be enabled");
    
    enhanced_vm_enable_gc(evm, true);
    TEST_ASSERT(evm->enable_gc, "GC should be enabled");
    
    enhanced_vm_enable_profiling(evm, true);
    TEST_ASSERT(evm->enable_profiling, "Profiling should be enabled");
    
    enhanced_vm_destroy(evm);
    TEST_MEMORY_VERIFY("Enhanced VM configuration should not leak memory");
    
    return true;
}

// Test suite definition
static test_function_t vm_core_tests[] = {
    test_vm_init_cleanup,
    test_vm_stack_operations,
    test_vm_stack_overflow,
    test_vm_reset,
    test_vm_interpretation,
    test_vm_interpretation_with_result,
    test_vm_arithmetic,
    test_vm_negation,
    test_enhanced_vm_lifecycle,
    test_enhanced_vm_configuration
};

test_suite_t vm_test_suite = {
    .name = "VM Core Tests",
    .tests = vm_core_tests,
    .test_count = sizeof(vm_core_tests) / sizeof(vm_core_tests[0])
};
