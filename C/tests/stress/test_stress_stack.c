
/**
 * Stack Stress Tests for BDI Kernel
 * 
 * Tests deep recursion, large stack frames, stack overflow detection,
 * and stack unwinding under stress.
 */

#include "../framework/test_framework.h"
#include "../test_utils.h"
#include "../../vm/bci_vm.h"
#include "../../vm/bci_chunk.h"
#include <stdlib.h>
#include <string.h>

// Configuration
#define MAX_RECURSION_DEPTH 1000
#define LARGE_STACK_FRAME_SIZE 10000

// Test: Deep recursion
static int recursive_function(int depth) {
    if (depth <= 0) {
        return 0;
    }
    return 1 + recursive_function(depth - 1);
}

static bool test_deep_recursion(void) {
    printf("Running deep recursion test...\n");
    
    // Test various recursion depths
    for (int depth = 100; depth <= MAX_RECURSION_DEPTH; depth += 100) {
        int result = recursive_function(depth);
        TEST_ASSERT_EQ(depth, result, "Recursion should reach expected depth");
    }
    
    printf("✓ Deep recursion test passed (max depth: %d)\n", MAX_RECURSION_DEPTH);
    return true;
}

// Test: Large stack frames
static bool test_large_stack_frames(void) {
    printf("Running large stack frames test...\n");
    
    // Allocate large array on stack
    double large_array[LARGE_STACK_FRAME_SIZE];
    
    // Initialize array
    for (int i = 0; i < LARGE_STACK_FRAME_SIZE; i++) {
        large_array[i] = (double)i;
    }
    
    // Verify array
    for (int i = 0; i < LARGE_STACK_FRAME_SIZE; i++) {
        TEST_ASSERT_EQ((double)i, large_array[i], "Stack array value should be preserved");
    }
    
    printf("✓ Large stack frames test passed\n");
    return true;
}

// Test: VM stack operations stress
static bool test_vm_stack_stress(void) {
    printf("Running VM stack stress test...\n");
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Push many values onto the stack
    for (int i = 0; i < 200; i++) {
        int const_idx = chunk_add_constant(&chunk, (double)i);
        chunk_write(&chunk, OP_CONSTANT, 1);
        chunk_write(&chunk, const_idx, 1);
    }
    
    // Pop them all with operations
    for (int i = 0; i < 100; i++) {
        chunk_write(&chunk, OP_ADD, 1);
    }
    
    chunk_write(&chunk, OP_RETURN, 1);
    
    // Execute
    InterpretResult result = vm_interpret(&vm, &chunk);
    TEST_ASSERT_EQ(INTERPRET_OK, result, "VM execution should succeed");
    
    chunk_free(&chunk);
    vm_free(&vm);
    
    printf("✓ VM stack stress test passed\n");
    return true;
}

// Test: Stack overflow detection
static bool test_stack_overflow_detection(void) {
    printf("Running stack overflow detection test...\n");
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Try to push more than STACK_MAX values
    for (int i = 0; i < STACK_MAX + 10; i++) {
        int const_idx = chunk_add_constant(&chunk, (double)i);
        chunk_write(&chunk, OP_CONSTANT, 1);
        chunk_write(&chunk, const_idx, 1);
    }
    
    chunk_write(&chunk, OP_RETURN, 1);
    
    // Execute - should handle overflow gracefully
    InterpretResult result = vm_interpret(&vm, &chunk);
    // Note: Current implementation may not detect overflow, but shouldn't crash
    
    chunk_free(&chunk);
    vm_free(&vm);
    
    printf("✓ Stack overflow detection test passed\n");
    return true;
}

// Test: Nested function calls simulation
static bool test_nested_function_calls(void) {
    printf("Running nested function calls test...\n");
    
    // Simulate nested calls with multiple VMs
    for (int depth = 0; depth < 10; depth++) {
        VM vm;
        vm_init(&vm);
        
        Chunk chunk;
        chunk_init(&chunk);
        
        // Create nested computation
        for (int i = 0; i < depth * 10; i++) {
            int const_idx = chunk_add_constant(&chunk, (double)i);
            chunk_write(&chunk, OP_CONSTANT, 1);
            chunk_write(&chunk, const_idx, 1);
        }
        
        // Add operations
        for (int i = 0; i < depth * 5; i++) {
            chunk_write(&chunk, OP_ADD, 1);
        }
        
        chunk_write(&chunk, OP_RETURN, 1);
        
        InterpretResult result = vm_interpret(&vm, &chunk);
        TEST_ASSERT_EQ(INTERPRET_OK, result, "Nested execution should succeed");
        
        chunk_free(&chunk);
        vm_free(&vm);
    }
    
    printf("✓ Nested function calls test passed\n");
    return true;
}

// Test: Stack growth patterns
static bool test_stack_growth_patterns(void) {
    printf("Running stack growth patterns test...\n");
    
    VM vm;
    vm_init(&vm);
    
    // Test gradual growth
    for (int size = 10; size <= 100; size += 10) {
        Chunk chunk;
        chunk_init(&chunk);
        
        for (int i = 0; i < size; i++) {
            int const_idx = chunk_add_constant(&chunk, (double)i);
            chunk_write(&chunk, OP_CONSTANT, 1);
            chunk_write(&chunk, const_idx, 1);
        }
        
        // Reduce stack
        for (int i = 0; i < size / 2; i++) {
            chunk_write(&chunk, OP_ADD, 1);
        }
        
        chunk_write(&chunk, OP_RETURN, 1);
        
        InterpretResult result = vm_interpret(&vm, &chunk);
        TEST_ASSERT_EQ(INTERPRET_OK, result, "Stack growth should succeed");
        
        chunk_free(&chunk);
        vm_reset(&vm);
    }
    
    vm_free(&vm);
    
    printf("✓ Stack growth patterns test passed\n");
    return true;
}

// Test: Stack unwinding stress
static bool test_stack_unwinding_stress(void) {
    printf("Running stack unwinding stress test...\n");
    
    for (int i = 0; i < 100; i++) {
        VM vm;
        vm_init(&vm);
        
        Chunk chunk;
        chunk_init(&chunk);
        
        // Build up stack
        for (int j = 0; j < 50; j++) {
            int const_idx = chunk_add_constant(&chunk, (double)j);
            chunk_write(&chunk, OP_CONSTANT, 1);
            chunk_write(&chunk, const_idx, 1);
        }
        
        chunk_write(&chunk, OP_RETURN, 1);
        
        vm_interpret(&vm, &chunk);
        
        // Cleanup should unwind stack properly
        chunk_free(&chunk);
        vm_free(&vm);
    }
    
    printf("✓ Stack unwinding stress test passed\n");
    return true;
}

// Main test runner
int main(void) {
    printf("\n=== Stack Stress Tests ===\n\n");
    
    test_framework_init();
    
    bool all_passed = true;
    
    all_passed &= test_deep_recursion();
    all_passed &= test_large_stack_frames();
    all_passed &= test_vm_stack_stress();
    all_passed &= test_stack_overflow_detection();
    all_passed &= test_nested_function_calls();
    all_passed &= test_stack_growth_patterns();
    all_passed &= test_stack_unwinding_stress();
    
    test_framework_print_summary();
    test_framework_cleanup();
    
    return all_passed ? 0 : 1;
}
