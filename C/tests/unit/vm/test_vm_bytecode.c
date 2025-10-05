
#include "../../framework/test_framework.h"
#include "../../../vm/bci_vm.h"
#include "../../../vm/bci_chunk.h"

// Test bytecode chunk creation and management
static bool test_bytecode_chunk_creation(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Test initial state - lazy initialization means pointers start as NULL
    TEST_ASSERT_EQ(0, chunk.count, "New chunk should be empty");
    TEST_ASSERT_EQ(0, chunk.capacity, "New chunk should have zero capacity");
    TEST_ASSERT_EQ(0, chunk.constants.len, "New chunk should have no constants");
    TEST_ASSERT_NULL(chunk.code, "Chunk code array should be NULL until first write (lazy initialization)");
    TEST_ASSERT_NULL(chunk.lines, "Chunk lines array should be NULL until first write (lazy initialization)");
    
    // Test lazy initialization - memory allocated on first write
    chunk_write(&chunk, OP_RETURN, 1);
    TEST_ASSERT_NOT_NULL(chunk.code, "Chunk code array should be allocated after first write");
    TEST_ASSERT_NOT_NULL(chunk.lines, "Chunk lines array should be allocated after first write");
    TEST_ASSERT_EQ(1, chunk.count, "Chunk should contain one byte after write");
    TEST_ASSERT(chunk.capacity >= 1, "Chunk capacity should be at least 1 after write");
    TEST_ASSERT_EQ(OP_RETURN, chunk.code[0], "Written byte should be stored correctly");
    TEST_ASSERT_EQ(1, chunk.lines[0], "Line number should be stored correctly");
    
    chunk_free(&chunk);
    TEST_MEMORY_VERIFY("Chunk creation should not leak memory");
    
    return true;
}

// Test bytecode instruction writing
static bool test_bytecode_instruction_writing(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Write various instructions using available API
    int constant_index = chunk_add_constant(&chunk, 42.0);
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, constant_index, 1);
    chunk_write(&chunk, OP_ADD, 2);
    chunk_write(&chunk, OP_RETURN, 3);
    
    TEST_ASSERT_EQ(4, chunk.count, "Chunk should contain 4 bytes");
    
    // Verify instructions using direct access to chunk structure
    TEST_ASSERT_EQ(OP_CONSTANT, chunk.code[0], "First instruction should be OP_CONSTANT");
    TEST_ASSERT_EQ(constant_index, chunk.code[1], "Second byte should be constant index");
    TEST_ASSERT_EQ(OP_ADD, chunk.code[2], "Third instruction should be OP_ADD");
    TEST_ASSERT_EQ(OP_RETURN, chunk.code[3], "Fourth instruction should be OP_RETURN");
    
    chunk_free(&chunk);
    TEST_MEMORY_VERIFY("Bytecode writing should not leak memory");
    
    return true;
}

// Test constant pool management
static bool test_constant_pool(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Add constants using available API
    int const1_idx = chunk_add_constant(&chunk, 42.0);
    int const2_idx = chunk_add_constant(&chunk, 3.14);
    int const3_idx = chunk_add_constant(&chunk, -1.5);
    
    TEST_ASSERT_EQ(0, const1_idx, "First constant should have index 0");
    TEST_ASSERT_EQ(1, const2_idx, "Second constant should have index 1");
    TEST_ASSERT_EQ(2, const3_idx, "Third constant should have index 2");
    TEST_ASSERT_EQ(3, chunk.constants.len, "Chunk should have 3 constants");
    
    // Retrieve constants using direct access
    TEST_ASSERT_EQ(42.0, chunk.constants.data[0], "First constant should be 42.0");
    TEST_ASSERT_EQ(3.14, chunk.constants.data[1], "Second constant should be 3.14");
    TEST_ASSERT_EQ(-1.5, chunk.constants.data[2], "Third constant should be -1.5");
    
    chunk_free(&chunk);
    TEST_MEMORY_VERIFY("Constant pool should not leak memory");
    
    return true;
}

// Test line number tracking
static bool test_line_number_tracking(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Write instructions with line numbers
    chunk_write(&chunk, OP_CONSTANT, 10);
    chunk_write(&chunk, 0, 10);
    chunk_write(&chunk, OP_CONSTANT, 11);
    chunk_write(&chunk, 1, 11);
    chunk_write(&chunk, OP_ADD, 12);
    chunk_write(&chunk, OP_RETURN, 13);
    
    // Verify line numbers using direct access
    TEST_ASSERT_EQ(10, chunk.lines[0], "First instruction should be on line 10");
    TEST_ASSERT_EQ(10, chunk.lines[1], "Second byte should be on line 10");
    TEST_ASSERT_EQ(11, chunk.lines[2], "Third instruction should be on line 11");
    TEST_ASSERT_EQ(11, chunk.lines[3], "Fourth byte should be on line 11");
    TEST_ASSERT_EQ(12, chunk.lines[4], "Fifth instruction should be on line 12");
    TEST_ASSERT_EQ(13, chunk.lines[5], "Sixth instruction should be on line 13");
    
    chunk_free(&chunk);
    TEST_MEMORY_VERIFY("Line number tracking should not leak memory");
    
    return true;
}

// Test bytecode execution with VM
static bool test_bytecode_execution(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Create a simple program: push 42, return
    int const_idx = chunk_add_constant(&chunk, 42.0);
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, const_idx, 1);
    chunk_write(&chunk, OP_RETURN, 1);
    
    // Execute bytecode
    InterpretResult result = vm_interpret(&vm, &chunk);
    TEST_ASSERT_EQ(INTERPRET_OK, result, "Simple bytecode should execute successfully");
    
    chunk_free(&chunk);
    vm_free(&vm);
    TEST_MEMORY_VERIFY("Bytecode execution should not leak memory");
    
    return true;
}

// Test complex bytecode with arithmetic
static bool test_complex_bytecode(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Create program: (5 + 3) * 2 - 1 = 15
    int const1 = chunk_add_constant(&chunk, 5.0);
    int const2 = chunk_add_constant(&chunk, 3.0);
    int const3 = chunk_add_constant(&chunk, 2.0);
    int const4 = chunk_add_constant(&chunk, 1.0);
    
    chunk_write(&chunk, OP_CONSTANT, 1);  // Push 5
    chunk_write(&chunk, const1, 1);
    chunk_write(&chunk, OP_CONSTANT, 2);  // Push 3
    chunk_write(&chunk, const2, 2);
    chunk_write(&chunk, OP_ADD, 3);       // 5 + 3 = 8
    chunk_write(&chunk, OP_CONSTANT, 4);  // Push 2
    chunk_write(&chunk, const3, 4);
    chunk_write(&chunk, OP_MULTIPLY, 5);  // 8 * 2 = 16
    chunk_write(&chunk, OP_CONSTANT, 6);  // Push 1
    chunk_write(&chunk, const4, 6);
    chunk_write(&chunk, OP_SUBTRACT, 7);  // 16 - 1 = 15
    chunk_write(&chunk, OP_RETURN, 8);
    
    // Execute complex bytecode
    BciVmResult result = vm_interpret_with_result(&vm, &chunk);
    TEST_ASSERT_EQ(INTERPRET_OK, result.status, "Complex bytecode should execute successfully");
    TEST_ASSERT_EQ(15.0, result.result_value, "Result should be 15.0");
    
    chunk_free(&chunk);
    vm_free(&vm);
    TEST_MEMORY_VERIFY("Complex bytecode should not leak memory");
    
    return true;
}

// Test bytecode with negation
static bool test_bytecode_negation(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Create program: -(5 + 3) = -8
    int const1 = chunk_add_constant(&chunk, 5.0);
    int const2 = chunk_add_constant(&chunk, 3.0);
    
    chunk_write(&chunk, OP_CONSTANT, 1);  // Push 5
    chunk_write(&chunk, const1, 1);
    chunk_write(&chunk, OP_CONSTANT, 2);  // Push 3
    chunk_write(&chunk, const2, 2);
    chunk_write(&chunk, OP_ADD, 3);       // 5 + 3 = 8
    chunk_write(&chunk, OP_NEGATE, 4);    // -8
    chunk_write(&chunk, OP_RETURN, 5);
    
    // Execute negation bytecode
    BciVmResult result = vm_interpret_with_result(&vm, &chunk);
    TEST_ASSERT_EQ(INTERPRET_OK, result.status, "Negation bytecode should execute successfully");
    TEST_ASSERT_EQ(-8.0, result.result_value, "Result should be -8.0");
    
    chunk_free(&chunk);
    vm_free(&vm);
    TEST_MEMORY_VERIFY("Negation bytecode should not leak memory");
    
    return true;
}

// Test bytecode with division
static bool test_bytecode_division(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Create program: 15 / 3 = 5
    int const1 = chunk_add_constant(&chunk, 15.0);
    int const2 = chunk_add_constant(&chunk, 3.0);
    
    chunk_write(&chunk, OP_CONSTANT, 1);  // Push 15
    chunk_write(&chunk, const1, 1);
    chunk_write(&chunk, OP_CONSTANT, 2);  // Push 3
    chunk_write(&chunk, const2, 2);
    chunk_write(&chunk, OP_DIVIDE, 3);    // 15 / 3 = 5
    chunk_write(&chunk, OP_RETURN, 4);
    
    // Execute division bytecode
    BciVmResult result = vm_interpret_with_result(&vm, &chunk);
    TEST_ASSERT_EQ(INTERPRET_OK, result.status, "Division bytecode should execute successfully");
    TEST_ASSERT_EQ(5.0, result.result_value, "Result should be 5.0");
    
    chunk_free(&chunk);
    vm_free(&vm);
    TEST_MEMORY_VERIFY("Division bytecode should not leak memory");
    
    return true;
}

// Test chunk capacity growth
static bool test_chunk_capacity_growth(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Chunk chunk;
    chunk_init(&chunk);
    
    int initial_capacity = chunk.capacity; // This is 0 due to lazy initialization
    TEST_ASSERT_EQ(0, initial_capacity, "Initial capacity should be 0 (lazy initialization)");
    
    // Write enough instructions to trigger multiple capacity growths
    // First write triggers initial allocation (capacity becomes 8)
    // Then we write more to trigger growth
    int instructions_to_write = 20; // This will trigger multiple growths
    for (int i = 0; i < instructions_to_write; i++) {
        chunk_write(&chunk, OP_RETURN, i + 1);
    }
    
    TEST_ASSERT(chunk.capacity > initial_capacity, "Chunk capacity should grow from initial 0");
    TEST_ASSERT(chunk.capacity >= instructions_to_write, "Capacity should accommodate all instructions");
    TEST_ASSERT_EQ(instructions_to_write, chunk.count, "All instructions should be written");
    
    // Verify that capacity growth follows the expected pattern (starts at 8, then doubles)
    TEST_ASSERT(chunk.capacity >= 8, "Minimum capacity should be at least 8 after first allocation");
    
    chunk_free(&chunk);
    TEST_MEMORY_VERIFY("Chunk capacity growth should not leak memory");
    
    return true;
}

// Test constant pool capacity growth
static bool test_constant_pool_growth(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Chunk chunk;
    chunk_init(&chunk);
    
    int initial_capacity = chunk.constants.capacity; // This is 0 due to lazy initialization
    TEST_ASSERT_EQ(0, initial_capacity, "Initial constants capacity should be 0 (lazy initialization)");
    
    // Add enough constants to trigger multiple capacity growths
    // First addition triggers initial allocation (capacity becomes 8)
    // Then we add more to trigger growth
    int constants_to_add = 20; // This will trigger multiple growths
    for (int i = 0; i < constants_to_add; i++) {
        chunk_add_constant(&chunk, (double)i);
    }
    
    TEST_ASSERT(chunk.constants.capacity > initial_capacity, "Constants capacity should grow from initial 0");
    TEST_ASSERT(chunk.constants.capacity >= constants_to_add, "Capacity should accommodate all constants");
    TEST_ASSERT_EQ(constants_to_add, chunk.constants.len, "All constants should be added");
    
    // Verify that capacity growth follows the expected pattern (starts at 8, then doubles)
    TEST_ASSERT(chunk.constants.capacity >= 8, "Minimum capacity should be at least 8 after first allocation");
    
    // Verify all constants are correct
    for (int i = 0; i < chunk.constants.len; i++) {
        TEST_ASSERT_EQ((double)i, chunk.constants.data[i], "Constant values should be preserved");
    }
    
    chunk_free(&chunk);
    TEST_MEMORY_VERIFY("Constant pool growth should not leak memory");
    
    return true;
}

// Test suite definition
static test_function_t vm_bytecode_tests[] = {
    test_bytecode_chunk_creation,
    test_bytecode_instruction_writing,
    test_constant_pool,
    test_line_number_tracking,
    test_bytecode_execution,
    test_complex_bytecode,
    test_bytecode_negation,
    test_bytecode_division,
    test_chunk_capacity_growth,
    test_constant_pool_growth
};

test_suite_t vm_bytecode_test_suite = {
    .name = "VM Bytecode Tests",
    .tests = vm_bytecode_tests,
    .test_count = sizeof(vm_bytecode_tests) / sizeof(vm_bytecode_tests[0])
};
