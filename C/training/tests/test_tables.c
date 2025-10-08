
#include "../include/training_tables.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

void test_table_common() {
    printf("Testing table common utilities...\n");
    
    // Test checksum
    const char *data = "Hello, World!";
    uint32_t checksum = table_calculate_checksum(data, strlen(data));
    assert(checksum != 0);
    
    printf("✓ Table common utilities test passed\n");
}

void test_math_tables() {
    printf("Testing math table generation...\n");
    
    int result;
    
    result = generate_math_arithmetic_table("/tmp/test_math_arithmetic.dat");
    assert(result == 0);
    
    result = table_validate_file("/tmp/test_math_arithmetic.dat");
    assert(result == 0);
    
    printf("✓ Math tables test passed\n");
}

void test_logic_tables() {
    printf("Testing logic table generation...\n");
    
    int result;
    
    result = generate_logic_boolean_table("/tmp/test_logic_boolean.dat");
    assert(result == 0);
    
    result = table_validate_file("/tmp/test_logic_boolean.dat");
    assert(result == 0);
    
    printf("✓ Logic tables test passed\n");
}

void test_language_tables() {
    printf("Testing language table generation...\n");
    
    int result;
    
    result = generate_language_ngrams_table("/tmp/test_language_ngrams.dat", NULL);
    assert(result == 0);
    
    result = table_validate_file("/tmp/test_language_ngrams.dat");
    assert(result == 0);
    
    printf("✓ Language tables test passed\n");
}

void test_code_tables() {
    printf("Testing code table generation...\n");
    
    int result;
    
    result = generate_code_ast_table("/tmp/test_code_ast.dat");
    assert(result == 0);
    
    result = table_validate_file("/tmp/test_code_ast.dat");
    assert(result == 0);
    
    printf("✓ Code tables test passed\n");
}

int main() {
    printf("=== Running Training Table Tests ===\n\n");
    
    test_table_common();
    test_math_tables();
    test_logic_tables();
    test_language_tables();
    test_code_tables();
    
    printf("\n=== All tests passed! ===\n");
    return 0;
}
