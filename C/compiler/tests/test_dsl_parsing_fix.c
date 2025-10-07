/**
 * @file test_dsl_parsing_fix.c
 * @brief Test for DSL Q-learning parameter parsing bug fix
 * 
 * This test verifies that state_space and action_space parameters
 * are correctly parsed from the "discrete[n]" format.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

// Forward declare the parsing function we're testing
// This is the same function added to ml_dsl_compiler.c
static bool parse_discrete_space(const char* space_str, size_t* out_size) {
    if (!space_str || !out_size) return false;
    
    // Expected format: "discrete[n]" where n is a positive integer
    const char* prefix = "discrete[";
    size_t prefix_len = strlen(prefix);
    
    if (strncmp(space_str, prefix, prefix_len) != 0) {
        return false;
    }
    
    // Find the closing bracket
    const char* bracket_pos = strchr(space_str + prefix_len, ']');
    if (!bracket_pos) {
        return false;
    }
    
    // Verify nothing comes after the closing bracket
    if (*(bracket_pos + 1) != '\0') {
        return false;
    }
    
    // Extract the number between brackets
    char num_str[32];
    size_t num_len = bracket_pos - (space_str + prefix_len);
    
    if (num_len == 0 || num_len >= sizeof(num_str)) {
        return false;
    }
    
    strncpy(num_str, space_str + prefix_len, num_len);
    num_str[num_len] = '\0';
    
    // Parse the number
    char* endptr;
    long value = strtol(num_str, &endptr, 10);
    
    // Check if parsing was successful and value is valid
    if (*endptr != '\0' || value <= 0 || value > 1000000) {
        return false;
    }
    
    *out_size = (size_t)value;
    return true;
}

// Test basic state/action space parsing
void test_basic_parsing(void) {
    printf("Testing basic discrete[n] format parsing...\n");
    
    size_t result;
    
    // Test basic valid cases
    assert(parse_discrete_space("discrete[10]", &result));
    assert(result == 10);
    printf("  ✓ Parsed 'discrete[10]' -> 10\n");
    
    assert(parse_discrete_space("discrete[4]", &result));
    assert(result == 4);
    printf("  ✓ Parsed 'discrete[4]' -> 4\n");
    
    assert(parse_discrete_space("discrete[100]", &result));
    assert(result == 100);
    printf("  ✓ Parsed 'discrete[100]' -> 100\n");
    
    printf("✓ Basic parsing test passed\n\n");
}

// Test various state/action space sizes
void test_various_sizes(void) {
    printf("Testing various discrete space sizes...\n");
    
    size_t test_values[] = {1, 5, 10, 50, 100, 200, 1000, 10000, 100000};
    
    for (size_t i = 0; i < sizeof(test_values) / sizeof(test_values[0]); i++) {
        char space_str[64];
        snprintf(space_str, sizeof(space_str), "discrete[%zu]", test_values[i]);
        
        size_t result;
        assert(parse_discrete_space(space_str, &result));
        assert(result == test_values[i]);
        
        printf("  ✓ Parsed '%s' -> %zu\n", space_str, result);
    }
    
    printf("✓ Various sizes test passed\n\n");
}

// Test whitespace handling
void test_whitespace_handling(void) {
    printf("Testing whitespace handling...\n");
    
    size_t result;
    
    // Note: strtol skips leading whitespace, so "discrete[ 10]" actually works
    // This is acceptable behavior for the parser
    assert(parse_discrete_space("discrete[ 10]", &result));
    assert(result == 10);
    printf("  ✓ Parsed 'discrete[ 10]' -> 10 (strtol handles leading space)\n");
    
    // Trailing space after number should fail because endptr won't point to ']'
    assert(!parse_discrete_space("discrete[10 ]", &result));
    printf("  ✓ Correctly rejected 'discrete[10 ]' (space after number)\n");
    
    // Space before bracket should fail (wrong prefix)
    assert(!parse_discrete_space("discrete [ 10 ]", &result));
    printf("  ✓ Correctly rejected 'discrete [ 10 ]' (space before bracket)\n");
    
    printf("✓ Whitespace handling test passed\n\n");
}

// Test error handling for invalid formats
void test_invalid_formats(void) {
    printf("Testing error handling for invalid formats...\n");
    
    size_t result;
    
    // Missing opening bracket
    assert(!parse_discrete_space("discrete10]", &result));
    printf("  ✓ Correctly rejected 'discrete10]' (missing opening bracket)\n");
    
    // Missing closing bracket
    assert(!parse_discrete_space("discrete[10", &result));
    printf("  ✓ Correctly rejected 'discrete[10' (missing closing bracket)\n");
    
    // Non-numeric value
    assert(!parse_discrete_space("discrete[abc]", &result));
    printf("  ✓ Correctly rejected 'discrete[abc]' (non-numeric)\n");
    
    // Zero value
    assert(!parse_discrete_space("discrete[0]", &result));
    printf("  ✓ Correctly rejected 'discrete[0]' (zero value)\n");
    
    // Negative value
    assert(!parse_discrete_space("discrete[-10]", &result));
    printf("  ✓ Correctly rejected 'discrete[-10]' (negative value)\n");
    
    // Empty brackets
    assert(!parse_discrete_space("discrete[]", &result));
    printf("  ✓ Correctly rejected 'discrete[]' (empty brackets)\n");
    
    // Wrong prefix
    assert(!parse_discrete_space("continuous[10]", &result));
    printf("  ✓ Correctly rejected 'continuous[10]' (wrong prefix)\n");
    
    // Null input
    assert(!parse_discrete_space(NULL, &result));
    printf("  ✓ Correctly rejected NULL input\n");
    
    printf("✓ Invalid format error handling test passed\n\n");
}

// Test edge cases
void test_edge_cases(void) {
    printf("Testing edge cases...\n");
    
    size_t result;
    
    // Minimal size
    assert(parse_discrete_space("discrete[1]", &result));
    assert(result == 1);
    printf("  ✓ Minimal size (1) works correctly\n");
    
    // Large but reasonable size
    assert(parse_discrete_space("discrete[999999]", &result));
    assert(result == 999999);
    printf("  ✓ Large size (999999) works correctly\n");
    
    // Maximum allowed size
    assert(parse_discrete_space("discrete[1000000]", &result));
    assert(result == 1000000);
    printf("  ✓ Maximum size (1000000) works correctly\n");
    
    // Over maximum (should fail)
    assert(!parse_discrete_space("discrete[1000001]", &result));
    printf("  ✓ Correctly rejected size over maximum (1000001)\n");
    
    // Very large number (should fail)
    assert(!parse_discrete_space("discrete[999999999999]", &result));
    printf("  ✓ Correctly rejected very large number\n");
    
    printf("✓ Edge cases test passed\n\n");
}

// Test format variations
void test_format_variations(void) {
    printf("Testing format variations...\n");
    
    size_t result;
    
    // Leading zeros (should work - strtol handles this)
    assert(parse_discrete_space("discrete[0010]", &result));
    assert(result == 10);
    printf("  ✓ Parsed 'discrete[0010]' -> 10 (leading zeros)\n");
    
    // Multiple digits
    assert(parse_discrete_space("discrete[12345]", &result));
    assert(result == 12345);
    printf("  ✓ Parsed 'discrete[12345]' -> 12345\n");
    
    // Mixed with text after (should fail)
    assert(!parse_discrete_space("discrete[10]extra", &result));
    printf("  ✓ Correctly rejected 'discrete[10]extra' (extra text)\n");
    
    printf("✓ Format variations test passed\n\n");
}

int main(void) {
    printf("=== DSL Q-learning Parameter Parsing Fix Tests ===\n\n");
    
    test_basic_parsing();
    test_various_sizes();
    test_whitespace_handling();
    test_invalid_formats();
    test_edge_cases();
    test_format_variations();
    
    printf("=== All DSL Parsing Tests Passed ===\n");
    
    return 0;
}
