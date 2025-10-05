
/**
 * VM Regression Tests for BDI Kernel
 * 
 * Comprehensive regression tests for VM functionality including bytecode execution,
 * value system, memory management, stack operations, and error handling.
 */

#include "../framework/test_framework.h"
#include "../test_utils.h"
#include "../../vm/bci_vm.h"
#include "../../vm/bci_chunk.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Test: Basic VM initialization and cleanup
static bool test_vm_init_free(void) {
    printf("Testing VM initialization and cleanup...\n");
    
    VM vm;
    vm_init(&vm);
    
    TEST_ASSERT_NOT_NULL(vm.stack_top, "Stack top should be initialized");
    TEST_ASSERT_NULL(vm.chunk, "Chunk should be NULL initially");
    TEST_ASSERT_NULL(vm.ip, "IP should be NULL initially");
    
    vm_free(&vm);
    
    printf("✓ VM init/free test passed\n");
    return true;
}

// Test: Chunk initialization and cleanup
static bool test_chunk_init_free(void) {
    printf("Testing chunk initialization and cleanup...\n");
    
    Chunk chunk;
    chunk_init(&chunk);
    
    TEST_ASSERT_EQ(0, chunk.count, "Chunk count should be 0");
    TEST_ASSERT_EQ(0, chunk.capacity, "Chunk capacity should be 0");
    TEST_ASSERT_NULL(chunk.code, "Chunk code should be NULL");
    
    chunk_free(&chunk);
    
    printf("✓ Chunk init/free test passed\n");
    return true;
}

// Test: Writing bytecode to chunk
static bool test_chunk_write(void) {
    printf("Testing chunk write operations...\n");
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Write some bytes
    chunk_write(&chunk, OP_RETURN, 1);
    chunk_write(&chunk, OP_CONSTANT, 2);
    chunk_write(&chunk, 0, 2);
    
    TEST_ASSERT_EQ(3, chunk.count, "Chunk should have 3 bytes");
    TEST_ASSERT_EQ(OP_RETURN, chunk.code[0], "First byte should be OP_RETURN");
    TEST_ASSERT_EQ(OP_CONSTANT, chunk.code[1], "Second byte should be OP_CONSTANT");
    TEST_ASSERT_EQ(0, chunk.code[2], "Third byte should be 0");
    
    chunk_free(&chunk);
    
    printf("✓ Chunk write test passed\n");
    return true;
}

// Test: Adding constants to chunk
static bool test_chunk_add_constant(void) {
    printf("Testing chunk constant addition...\n");
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Add constants
    int idx1 = chunk_add_constant(&chunk, 1.5);
    int idx2 = chunk_add_constant(&chunk, 2.5);
    int idx3 = chunk_add_constant(&chunk, 3.5);
    
    TEST_ASSERT_EQ(0, idx1, "First constant index should be 0");
    TEST_ASSERT_EQ(1, idx2, "Second constant index should be 1");
    TEST_ASSERT_EQ(2, idx3, "Third constant index should be 2");
    
    TEST_ASSERT_EQ(1.5, chunk.constants.data[0], "First constant should be 1.5");
    TEST_ASSERT_EQ(2.5, chunk.constants.data[1], "Second constant should be 2.5");
    TEST_ASSERT_EQ(3.5, chunk.constants.data[2], "Third constant should be 3.5");
    
    chunk_free(&chunk);
    
    printf("✓ Chunk add constant test passed\n");
    return true;
}

// Test: Simple constant execution
static bool test_vm_constant_execution(void) {
    printf("Testing VM constant execution...\n");
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    int const_idx = chunk_add_constant(&chunk, 42.0);
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, const_idx, 1);
    chunk_write(&chunk, OP_RETURN, 1);
    
    InterpretResult result = vm_interpret(&vm, &chunk);
    TEST_ASSERT_EQ(INTERPRET_OK, result, "Execution should succeed");
    
    chunk_free(&chunk);
    vm_free(&vm);
    
    printf("✓ VM constant execution test passed\n");
    return true;
}

// Test: Arithmetic operations
static bool test_vm_arithmetic_operations(void) {
    printf("Testing VM arithmetic operations...\n");
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Test: 2 + 3 = 5
    int idx1 = chunk_add_constant(&chunk, 2.0);
    int idx2 = chunk_add_constant(&chunk, 3.0);
    
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, idx1, 1);
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, idx2, 1);
    chunk_write(&chunk, OP_ADD, 1);
    chunk_write(&chunk, OP_RETURN, 1);
    
    InterpretResult result = vm_interpret(&vm, &chunk);
    TEST_ASSERT_EQ(INTERPRET_OK, result, "Addition should succeed");
    
    chunk_free(&chunk);
    vm_free(&vm);
    
    printf("✓ VM arithmetic operations test passed\n");
    return true;
}

// Test: Negation operation
static bool test_vm_negation(void) {
    printf("Testing VM negation operation...\n");
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Test: -5
    int idx = chunk_add_constant(&chunk, 5.0);
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, idx, 1);
    chunk_write(&chunk, OP_NEGATE, 1);
    chunk_write(&chunk, OP_RETURN, 1);
    
    InterpretResult result = vm_interpret(&vm, &chunk);
    TEST_ASSERT_EQ(INTERPRET_OK, result, "Negation should succeed");
    
    chunk_free(&chunk);
    vm_free(&vm);
    
    printf("✓ VM negation test passed\n");
    return true;
}

// Test: Complex arithmetic expression
static bool test_vm_complex_expression(void) {
    printf("Testing VM complex expression...\n");
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Test: (2 + 3) * 4 = 20
    int idx1 = chunk_add_constant(&chunk, 2.0);
    int idx2 = chunk_add_constant(&chunk, 3.0);
    int idx3 = chunk_add_constant(&chunk, 4.0);
    
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, idx1, 1);
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, idx2, 1);
    chunk_write(&chunk, OP_ADD, 1);
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, idx3, 1);
    chunk_write(&chunk, OP_MULTIPLY, 1);
    chunk_write(&chunk, OP_RETURN, 1);
    
    InterpretResult result = vm_interpret(&vm, &chunk);
    TEST_ASSERT_EQ(INTERPRET_OK, result, "Complex expression should succeed");
    
    chunk_free(&chunk);
    vm_free(&vm);
    
    printf("✓ VM complex expression test passed\n");
    return true;
}

// Test: Division operation
static bool test_vm_division(void) {
    printf("Testing VM division operation...\n");
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Test: 10 / 2 = 5
    int idx1 = chunk_add_constant(&chunk, 10.0);
    int idx2 = chunk_add_constant(&chunk, 2.0);
    
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, idx1, 1);
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, idx2, 1);
    chunk_write(&chunk, OP_DIVIDE, 1);
    chunk_write(&chunk, OP_RETURN, 1);
    
    InterpretResult result = vm_interpret(&vm, &chunk);
    TEST_ASSERT_EQ(INTERPRET_OK, result, "Division should succeed");
    
    chunk_free(&chunk);
    vm_free(&vm);
    
    printf("✓ VM division test passed\n");
    return true;
}

// Test: Subtraction operation
static bool test_vm_subtraction(void) {
    printf("Testing VM subtraction operation...\n");
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Test: 10 - 3 = 7
    int idx1 = chunk_add_constant(&chunk, 10.0);
    int idx2 = chunk_add_constant(&chunk, 3.0);
    
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, idx1, 1);
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, idx2, 1);
    chunk_write(&chunk, OP_SUBTRACT, 1);
    chunk_write(&chunk, OP_RETURN, 1);
    
    InterpretResult result = vm_interpret(&vm, &chunk);
    TEST_ASSERT_EQ(INTERPRET_OK, result, "Subtraction should succeed");
    
    chunk_free(&chunk);
    vm_free(&vm);
    
    printf("✓ VM subtraction test passed\n");
    return true;
}

// Test: Empty chunk execution
static bool test_vm_empty_chunk(void) {
    printf("Testing VM empty chunk execution...\n");
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    chunk_write(&chunk, OP_RETURN, 1);
    
    InterpretResult result = vm_interpret(&vm, &chunk);
    TEST_ASSERT_EQ(INTERPRET_OK, result, "Empty chunk should succeed");
    
    chunk_free(&chunk);
    vm_free(&vm);
    
    printf("✓ VM empty chunk test passed\n");
    return true;
}

// Test: Multiple VM resets
static bool test_vm_reset(void) {
    printf("Testing VM reset functionality...\n");
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    int idx = chunk_add_constant(&chunk, 42.0);
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, idx, 1);
    chunk_write(&chunk, OP_RETURN, 1);
    
    // Execute multiple times with reset
    for (int i = 0; i < 10; i++) {
        vm_reset(&vm);
        InterpretResult result = vm_interpret(&vm, &chunk);
        TEST_ASSERT_EQ(INTERPRET_OK, result, "Execution after reset should succeed");
    }
    
    chunk_free(&chunk);
    vm_free(&vm);
    
    printf("✓ VM reset test passed\n");
    return true;
}

// Test: Large constant pool
static bool test_vm_large_constant_pool(void) {
    printf("Testing VM with large constant pool...\n");
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Add many constants
    for (int i = 0; i < 256; i++) {
        chunk_add_constant(&chunk, (double)i);
    }
    
    // Use some constants
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, 0, 1);
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, 255, 1);
    chunk_write(&chunk, OP_ADD, 1);
    chunk_write(&chunk, OP_RETURN, 1);
    
    InterpretResult result = vm_interpret(&vm, &chunk);
    TEST_ASSERT_EQ(INTERPRET_OK, result, "Large constant pool should work");
    
    chunk_free(&chunk);
    vm_free(&vm);
    
    printf("✓ VM large constant pool test passed\n");
    return true;
}

// Test: Stack operations
static bool test_vm_stack_operations(void) {
    printf("Testing VM stack operations...\n");
    
    VM vm;
    vm_init(&vm);
    
    // Test push
    vm_stack_push(&vm, 1.0);
    vm_stack_push(&vm, 2.0);
    vm_stack_push(&vm, 3.0);
    
    // Test pop
    double val3 = vm_stack_pop(&vm);
    double val2 = vm_stack_pop(&vm);
    double val1 = vm_stack_pop(&vm);
    
    TEST_ASSERT_EQ(3.0, val3, "Popped value should be 3.0");
    TEST_ASSERT_EQ(2.0, val2, "Popped value should be 2.0");
    TEST_ASSERT_EQ(1.0, val1, "Popped value should be 1.0");
    
    vm_free(&vm);
    
    printf("✓ VM stack operations test passed\n");
    return true;
}

// Main test runner
int main(void) {
    printf("\n=== VM Regression Tests ===\n\n");
    
    test_framework_init();
    
    bool all_passed = true;
    
    all_passed &= test_vm_init_free();
    all_passed &= test_chunk_init_free();
    all_passed &= test_chunk_write();
    all_passed &= test_chunk_add_constant();
    all_passed &= test_vm_constant_execution();
    all_passed &= test_vm_arithmetic_operations();
    all_passed &= test_vm_negation();
    all_passed &= test_vm_complex_expression();
    all_passed &= test_vm_division();
    all_passed &= test_vm_subtraction();
    all_passed &= test_vm_empty_chunk();
    all_passed &= test_vm_reset();
    all_passed &= test_vm_large_constant_pool();
    all_passed &= test_vm_stack_operations();
    
    test_framework_print_summary();
    test_framework_cleanup();
    
    return all_passed ? 0 : 1;
}
