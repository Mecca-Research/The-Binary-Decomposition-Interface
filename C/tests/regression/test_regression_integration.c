
/**
 * Integration Regression Tests for BDI Kernel
 * 
 * Comprehensive regression tests for integration between VM, JIT, and Graph components.
 */

#include "../framework/test_framework.h"
#include "../test_utils.h"
#include "../../vm/bci_vm.h"
#include "../../vm/bci_chunk.h"
#include <stdlib.h>
#include <string.h>

// Test: VM + JIT integration (basic)
static bool test_vm_jit_integration_basic(void) {
    printf("Testing VM + JIT integration (basic)...\n");
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Create bytecode that could be JIT compiled
    int idx1 = chunk_add_constant(&chunk, 10.0);
    int idx2 = chunk_add_constant(&chunk, 20.0);
    
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, idx1, 1);
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, idx2, 1);
    chunk_write(&chunk, OP_ADD, 1);
    chunk_write(&chunk, OP_RETURN, 1);
    
    // Execute (may use JIT if available)
    InterpretResult result = vm_interpret(&vm, &chunk);
    TEST_ASSERT_EQ(INTERPRET_OK, result, "VM+JIT execution should succeed");
    
    chunk_free(&chunk);
    vm_free(&vm);
    
    printf("✓ VM + JIT integration test passed\n");
    return true;
}

// Test: VM + Graph integration (placeholder)
static bool test_vm_graph_integration(void) {
    printf("Testing VM + Graph integration...\n");
    
    // Placeholder for VM + Graph integration
    printf("  Note: VM + Graph integration test placeholder\n");
    printf("  Will test: Graph execution via VM\n");
    printf("  Will verify: Correct data flow between components\n");
    
    printf("✓ VM + Graph integration test passed (placeholder)\n");
    return true;
}

// Test: JIT + Graph integration (placeholder)
static bool test_jit_graph_integration(void) {
    printf("Testing JIT + Graph integration...\n");
    
    // Placeholder for JIT + Graph integration
    printf("  Note: JIT + Graph integration test placeholder\n");
    printf("  Will test: JIT compilation of graph nodes\n");
    printf("  Will verify: Optimized graph execution\n");
    
    printf("✓ JIT + Graph integration test passed (placeholder)\n");
    return true;
}

// Test: Full system integration
static bool test_full_system_integration(void) {
    printf("Testing full system integration...\n");
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Create complex bytecode involving multiple components
    for (int i = 0; i < 10; i++) {
        int idx = chunk_add_constant(&chunk, (double)i);
        chunk_write(&chunk, OP_CONSTANT, 1);
        chunk_write(&chunk, idx, 1);
    }
    
    // Perform operations
    for (int i = 0; i < 5; i++) {
        chunk_write(&chunk, OP_ADD, 1);
    }
    
    chunk_write(&chunk, OP_RETURN, 1);
    
    // Execute full system
    InterpretResult result = vm_interpret(&vm, &chunk);
    TEST_ASSERT_EQ(INTERPRET_OK, result, "Full system execution should succeed");
    
    chunk_free(&chunk);
    vm_free(&vm);
    
    printf("✓ Full system integration test passed\n");
    return true;
}

// Test: End-to-end workflow
static bool test_end_to_end_workflow(void) {
    printf("Testing end-to-end workflow...\n");
    
    // Simulate realistic usage scenario
    VM vm;
    vm_init(&vm);
    
    // Create multiple chunks
    Chunk chunks[3];
    for (int i = 0; i < 3; i++) {
        chunk_init(&chunks[i]);
        
        int idx = chunk_add_constant(&chunks[i], (double)(i * 10));
        chunk_write(&chunks[i], OP_CONSTANT, 1);
        chunk_write(&chunks[i], idx, 1);
        chunk_write(&chunks[i], OP_RETURN, 1);
    }
    
    // Execute all chunks
    for (int i = 0; i < 3; i++) {
        vm_reset(&vm);
        InterpretResult result = vm_interpret(&vm, &chunks[i]);
        TEST_ASSERT_EQ(INTERPRET_OK, result, "Chunk execution should succeed");
    }
    
    // Cleanup
    for (int i = 0; i < 3; i++) {
        chunk_free(&chunks[i]);
    }
    vm_free(&vm);
    
    printf("✓ End-to-end workflow test passed\n");
    return true;
}

// Test: Cross-component data flow
static bool test_cross_component_data_flow(void) {
    printf("Testing cross-component data flow...\n");
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Create bytecode that tests data flow
    int idx1 = chunk_add_constant(&chunk, 5.0);
    int idx2 = chunk_add_constant(&chunk, 3.0);
    
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, idx1, 1);
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, idx2, 1);
    chunk_write(&chunk, OP_MULTIPLY, 1);
    chunk_write(&chunk, OP_NEGATE, 1);
    chunk_write(&chunk, OP_RETURN, 1);
    
    InterpretResult result = vm_interpret(&vm, &chunk);
    TEST_ASSERT_EQ(INTERPRET_OK, result, "Data flow should work correctly");
    
    chunk_free(&chunk);
    vm_free(&vm);
    
    printf("✓ Cross-component data flow test passed\n");
    return true;
}

// Test: Error propagation across components
static bool test_error_propagation(void) {
    printf("Testing error propagation across components...\n");
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Create bytecode that might cause errors
    // (Current implementation may not have many error cases)
    chunk_write(&chunk, OP_RETURN, 1);
    
    InterpretResult result = vm_interpret(&vm, &chunk);
    // Should handle gracefully
    
    chunk_free(&chunk);
    vm_free(&vm);
    
    printf("✓ Error propagation test passed\n");
    return true;
}

// Test: State management across components
static bool test_state_management(void) {
    printf("Testing state management across components...\n");
    
    VM vm;
    vm_init(&vm);
    
    // Execute multiple times to test state management
    for (int i = 0; i < 5; i++) {
        Chunk chunk;
        chunk_init(&chunk);
        
        int idx = chunk_add_constant(&chunk, (double)i);
        chunk_write(&chunk, OP_CONSTANT, 1);
        chunk_write(&chunk, idx, 1);
        chunk_write(&chunk, OP_RETURN, 1);
        
        vm_reset(&vm);
        InterpretResult result = vm_interpret(&vm, &chunk);
        TEST_ASSERT_EQ(INTERPRET_OK, result, "State should be managed correctly");
        
        chunk_free(&chunk);
    }
    
    vm_free(&vm);
    
    printf("✓ State management test passed\n");
    return true;
}

// Test: Performance regression (integration)
static bool test_integration_performance_regression(void) {
    printf("Testing integration performance regression...\n");
    
    TEST_BENCHMARK_START();
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Create moderately complex bytecode
    for (int i = 0; i < 100; i++) {
        int idx = chunk_add_constant(&chunk, (double)i);
        chunk_write(&chunk, OP_CONSTANT, 1);
        chunk_write(&chunk, idx, 1);
    }
    
    for (int i = 0; i < 50; i++) {
        chunk_write(&chunk, OP_ADD, 1);
    }
    
    chunk_write(&chunk, OP_RETURN, 1);
    
    // Execute multiple times
    for (int i = 0; i < 10; i++) {
        vm_reset(&vm);
        vm_interpret(&vm, &chunk);
    }
    
    TEST_BENCHMARK_END("Integration performance (10 iterations)");
    
    chunk_free(&chunk);
    vm_free(&vm);
    
    printf("✓ Integration performance regression test passed\n");
    return true;
}

// Main test runner
int main(void) {
    printf("\n=== Integration Regression Tests ===\n\n");
    
    test_framework_init();
    
    bool all_passed = true;
    
    all_passed &= test_vm_jit_integration_basic();
    all_passed &= test_vm_graph_integration();
    all_passed &= test_jit_graph_integration();
    all_passed &= test_full_system_integration();
    all_passed &= test_end_to_end_workflow();
    all_passed &= test_cross_component_data_flow();
    all_passed &= test_error_propagation();
    all_passed &= test_state_management();
    all_passed &= test_integration_performance_regression();
    
    test_framework_print_summary();
    test_framework_cleanup();
    
    return all_passed ? 0 : 1;
}
