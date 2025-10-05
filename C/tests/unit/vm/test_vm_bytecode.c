
#include "../../framework/test_framework.h"
#include "../../../vm/bci_vm.h"
#include "../../../vm/bci_chunk.h"

// Test bytecode chunk creation and management
static bool test_bytecode_chunk_creation(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Chunk* chunk = chunk_create();
    TEST_ASSERT_NOT_NULL(chunk, "Chunk creation should succeed");
    
    TEST_ASSERT_EQ(0, chunk_size(chunk), "New chunk should be empty");
    TEST_ASSERT_EQ(0, chunk_constant_count(chunk), "New chunk should have no constants");
    
    chunk_destroy(chunk);
    TEST_MEMORY_VERIFY("Chunk creation should not leak memory");
    
    return true;
}

// Test bytecode instruction writing
static bool test_bytecode_instruction_writing(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Chunk* chunk = chunk_create();
    TEST_ASSERT_NOT_NULL(chunk, "Chunk creation should succeed");
    
    // Write various instructions
    chunk_write_byte(chunk, OP_CONSTANT, 1);
    chunk_write_byte(chunk, 0, 1); // constant index
    chunk_write_byte(chunk, OP_ADD, 2);
    chunk_write_byte(chunk, OP_RETURN, 3);
    
    TEST_ASSERT_EQ(4, chunk_size(chunk), "Chunk should contain 4 bytes");
    
    // Verify instructions
    TEST_ASSERT_EQ(OP_CONSTANT, chunk_get_byte(chunk, 0), "First instruction should be OP_CONSTANT");
    TEST_ASSERT_EQ(0, chunk_get_byte(chunk, 1), "Second byte should be constant index");
    TEST_ASSERT_EQ(OP_ADD, chunk_get_byte(chunk, 2), "Third instruction should be OP_ADD");
    TEST_ASSERT_EQ(OP_RETURN, chunk_get_byte(chunk, 3), "Fourth instruction should be OP_RETURN");
    
    chunk_destroy(chunk);
    TEST_MEMORY_VERIFY("Bytecode writing should not leak memory");
    
    return true;
}

// Test constant pool management
static bool test_constant_pool(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Chunk* chunk = chunk_create();
    TEST_ASSERT_NOT_NULL(chunk, "Chunk creation should succeed");
    
    // Add constants
    int const1_idx = chunk_add_constant(chunk, VALUE_NUMBER(42.0));
    int const2_idx = chunk_add_constant(chunk, VALUE_NUMBER(3.14));
    int const3_idx = chunk_add_constant(chunk, VALUE_BOOL(true));
    
    TEST_ASSERT_EQ(0, const1_idx, "First constant should have index 0");
    TEST_ASSERT_EQ(1, const2_idx, "Second constant should have index 1");
    TEST_ASSERT_EQ(2, const3_idx, "Third constant should have index 2");
    TEST_ASSERT_EQ(3, chunk_constant_count(chunk), "Chunk should have 3 constants");
    
    // Retrieve constants
    Value val1 = chunk_get_constant(chunk, 0);
    Value val2 = chunk_get_constant(chunk, 1);
    Value val3 = chunk_get_constant(chunk, 2);
    
    TEST_ASSERT(IS_NUMBER(val1), "First constant should be a number");
    TEST_ASSERT_EQ(42.0, AS_NUMBER(val1), "First constant should be 42.0");
    
    TEST_ASSERT(IS_NUMBER(val2), "Second constant should be a number");
    TEST_ASSERT_EQ(3.14, AS_NUMBER(val2), "Second constant should be 3.14");
    
    TEST_ASSERT(IS_BOOL(val3), "Third constant should be a boolean");
    TEST_ASSERT(AS_BOOL(val3), "Third constant should be true");
    
    chunk_destroy(chunk);
    TEST_MEMORY_VERIFY("Constant pool should not leak memory");
    
    return true;
}

// Test line number tracking
static bool test_line_number_tracking(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Chunk* chunk = chunk_create();
    TEST_ASSERT_NOT_NULL(chunk, "Chunk creation should succeed");
    
    // Write instructions with line numbers
    chunk_write_byte(chunk, OP_CONSTANT, 10);
    chunk_write_byte(chunk, 0, 10);
    chunk_write_byte(chunk, OP_CONSTANT, 11);
    chunk_write_byte(chunk, 1, 11);
    chunk_write_byte(chunk, OP_ADD, 12);
    chunk_write_byte(chunk, OP_RETURN, 13);
    
    // Verify line numbers
    TEST_ASSERT_EQ(10, chunk_get_line(chunk, 0), "First instruction should be on line 10");
    TEST_ASSERT_EQ(10, chunk_get_line(chunk, 1), "Second byte should be on line 10");
    TEST_ASSERT_EQ(11, chunk_get_line(chunk, 2), "Third instruction should be on line 11");
    TEST_ASSERT_EQ(11, chunk_get_line(chunk, 3), "Fourth byte should be on line 11");
    TEST_ASSERT_EQ(12, chunk_get_line(chunk, 4), "Fifth instruction should be on line 12");
    TEST_ASSERT_EQ(13, chunk_get_line(chunk, 5), "Sixth instruction should be on line 13");
    
    chunk_destroy(chunk);
    TEST_MEMORY_VERIFY("Line number tracking should not leak memory");
    
    return true;
}

// Test bytecode disassembly
static bool test_bytecode_disassembly(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Chunk* chunk = chunk_create();
    TEST_ASSERT_NOT_NULL(chunk, "Chunk creation should succeed");
    
    // Create a simple program
    int const_idx = chunk_add_constant(chunk, VALUE_NUMBER(42.0));
    chunk_write_byte(chunk, OP_CONSTANT, 1);
    chunk_write_byte(chunk, const_idx, 1);
    chunk_write_byte(chunk, OP_NEGATE, 2);
    chunk_write_byte(chunk, OP_RETURN, 3);
    
    // Test disassembly
    char* disassembly = chunk_disassemble(chunk, "test");
    TEST_ASSERT_NOT_NULL(disassembly, "Disassembly should succeed");
    
    // Check that disassembly contains expected elements
    TEST_ASSERT(strstr(disassembly, "OP_CONSTANT") != NULL, "Disassembly should contain OP_CONSTANT");
    TEST_ASSERT(strstr(disassembly, "OP_NEGATE") != NULL, "Disassembly should contain OP_NEGATE");
    TEST_ASSERT(strstr(disassembly, "OP_RETURN") != NULL, "Disassembly should contain OP_RETURN");
    TEST_ASSERT(strstr(disassembly, "42") != NULL, "Disassembly should contain constant value");
    
    free(disassembly);
    chunk_destroy(chunk);
    TEST_MEMORY_VERIFY("Bytecode disassembly should not leak memory");
    
    return true;
}

// Test bytecode validation
static bool test_bytecode_validation(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Chunk* chunk = chunk_create();
    TEST_ASSERT_NOT_NULL(chunk, "Chunk creation should succeed");
    
    // Create valid bytecode
    int const_idx = chunk_add_constant(chunk, VALUE_NUMBER(42.0));
    chunk_write_byte(chunk, OP_CONSTANT, 1);
    chunk_write_byte(chunk, const_idx, 1);
    chunk_write_byte(chunk, OP_RETURN, 2);
    
    // Validate bytecode
    bool is_valid = chunk_validate(chunk);
    TEST_ASSERT(is_valid, "Valid bytecode should pass validation");
    
    // Create invalid bytecode (missing return)
    Chunk* invalid_chunk = chunk_create();
    chunk_write_byte(invalid_chunk, OP_CONSTANT, 1);
    chunk_write_byte(invalid_chunk, const_idx, 1);
    // Missing OP_RETURN
    
    bool is_invalid = chunk_validate(invalid_chunk);
    TEST_ASSERT(!is_invalid, "Invalid bytecode should fail validation");
    
    chunk_destroy(chunk);
    chunk_destroy(invalid_chunk);
    TEST_MEMORY_VERIFY("Bytecode validation should not leak memory");
    
    return true;
}

// Test bytecode optimization
static bool test_bytecode_optimization(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Chunk* chunk = chunk_create();
    TEST_ASSERT_NOT_NULL(chunk, "Chunk creation should succeed");
    
    // Create unoptimized bytecode with redundant operations
    int const1 = chunk_add_constant(chunk, VALUE_NUMBER(5.0));
    int const2 = chunk_add_constant(chunk, VALUE_NUMBER(3.0));
    
    chunk_write_byte(chunk, OP_CONSTANT, 1);
    chunk_write_byte(chunk, const1, 1);
    chunk_write_byte(chunk, OP_CONSTANT, 2);
    chunk_write_byte(chunk, const2, 2);
    chunk_write_byte(chunk, OP_ADD, 3);
    chunk_write_byte(chunk, OP_POP, 4);  // Redundant pop
    chunk_write_byte(chunk, OP_CONSTANT, 5);
    chunk_write_byte(chunk, const1, 5);
    chunk_write_byte(chunk, OP_RETURN, 6);
    
    size_t original_size = chunk_size(chunk);
    
    // Optimize bytecode
    chunk_optimize(chunk);
    
    size_t optimized_size = chunk_size(chunk);
    TEST_ASSERT(optimized_size <= original_size, "Optimization should not increase size");
    
    // Verify optimization removed redundant operations
    bool has_redundant_pop = false;
    for (size_t i = 0; i < optimized_size; i++) {
        if (chunk_get_byte(chunk, i) == OP_POP) {
            // Check if this pop is actually redundant
            if (i > 0 && chunk_get_byte(chunk, i-1) == OP_ADD) {
                has_redundant_pop = true;
                break;
            }
        }
    }
    TEST_ASSERT(!has_redundant_pop, "Optimization should remove redundant operations");
    
    chunk_destroy(chunk);
    TEST_MEMORY_VERIFY("Bytecode optimization should not leak memory");
    
    return true;
}

// Test bytecode serialization
static bool test_bytecode_serialization(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Chunk* chunk = chunk_create();
    TEST_ASSERT_NOT_NULL(chunk, "Chunk creation should succeed");
    
    // Create bytecode
    int const_idx = chunk_add_constant(chunk, VALUE_NUMBER(42.0));
    chunk_write_byte(chunk, OP_CONSTANT, 1);
    chunk_write_byte(chunk, const_idx, 1);
    chunk_write_byte(chunk, OP_NEGATE, 2);
    chunk_write_byte(chunk, OP_RETURN, 3);
    
    // Serialize chunk
    size_t serialized_size;
    uint8_t* serialized_data = chunk_serialize(chunk, &serialized_size);
    TEST_ASSERT_NOT_NULL(serialized_data, "Serialization should succeed");
    TEST_ASSERT(serialized_size > 0, "Serialized data should have non-zero size");
    
    // Deserialize chunk
    Chunk* deserialized_chunk = chunk_deserialize(serialized_data, serialized_size);
    TEST_ASSERT_NOT_NULL(deserialized_chunk, "Deserialization should succeed");
    
    // Verify deserialized chunk matches original
    TEST_ASSERT_EQ(chunk_size(chunk), chunk_size(deserialized_chunk), "Sizes should match");
    TEST_ASSERT_EQ(chunk_constant_count(chunk), chunk_constant_count(deserialized_chunk), "Constant counts should match");
    
    for (size_t i = 0; i < chunk_size(chunk); i++) {
        TEST_ASSERT_EQ(chunk_get_byte(chunk, i), chunk_get_byte(deserialized_chunk, i), "Bytecode should match");
    }
    
    free(serialized_data);
    chunk_destroy(chunk);
    chunk_destroy(deserialized_chunk);
    TEST_MEMORY_VERIFY("Bytecode serialization should not leak memory");
    
    return true;
}

// Test suite definition
static test_function_t vm_bytecode_tests[] = {
    test_bytecode_chunk_creation,
    test_bytecode_instruction_writing,
    test_constant_pool,
    test_line_number_tracking,
    test_bytecode_disassembly,
    test_bytecode_validation,
    test_bytecode_optimization,
    test_bytecode_serialization
};

test_suite_t vm_bytecode_test_suite = {
    .name = "VM Bytecode Tests",
    .tests = vm_bytecode_tests,
    .test_count = sizeof(vm_bytecode_tests) / sizeof(vm_bytecode_tests[0])
};
