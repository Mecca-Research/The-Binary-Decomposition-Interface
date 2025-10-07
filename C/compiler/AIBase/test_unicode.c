
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "tables/unicode_tables.h"

// Test character properties
void test_char_properties(void) {
    printf("Testing character properties...\n");
    
    // Test ASCII letter
    assert(unicode_is_letter('A'));
    assert(unicode_is_letter('z'));
    assert(!unicode_is_digit('A'));
    
    // Test ASCII digit
    assert(unicode_is_digit('0'));
    assert(unicode_is_digit('9'));
    assert(!unicode_is_letter('0'));
    
    // Test whitespace
    assert(unicode_is_whitespace(' '));
    assert(unicode_is_whitespace('\t'));
    
    // Test punctuation
    assert(unicode_is_punctuation('.'));
    assert(unicode_is_punctuation(','));
    
    printf("  ✓ Character properties tests passed\n");
}

// Test case conversion
void test_case_conversion(void) {
    printf("Testing case conversion...\n");
    
    // ASCII case conversion
    assert(unicode_to_upper('a') == 'A');
    assert(unicode_to_lower('A') == 'a');
    assert(unicode_to_upper('z') == 'Z');
    assert(unicode_to_lower('Z') == 'z');
    
    // No change for non-letters
    assert(unicode_to_upper('0') == '0');
    assert(unicode_to_lower('0') == '0');
    
    printf("  ✓ Case conversion tests passed\n");
}

// Test UTF-8 conversion
void test_utf8_conversion(void) {
    printf("Testing UTF-8 conversion...\n");
    
    char utf8[5];
    
    // ASCII
    unicode_to_utf8('A', utf8);
    assert(strcmp(utf8, "A") == 0);
    
    // 2-byte UTF-8
    unicode_to_utf8(0x00E9, utf8); // é
    assert((unsigned char)utf8[0] == 0xC3);
    assert((unsigned char)utf8[1] == 0xA9);
    
    // 3-byte UTF-8
    unicode_to_utf8(0x4E2D, utf8); // 中
    assert((unsigned char)utf8[0] == 0xE4);
    assert((unsigned char)utf8[1] == 0xB8);
    assert((unsigned char)utf8[2] == 0xAD);
    
    // Test reverse conversion
    const char *p = "A";
    assert(unicode_from_utf8(&p) == 'A');
    
    printf("  ✓ UTF-8 conversion tests passed\n");
}

// Test string operations
void test_string_operations(void) {
    printf("Testing string operations...\n");
    
    // ASCII string
    const char *ascii = "Hello";
    assert(unicode_strlen(ascii) == 5);
    
    // Mixed ASCII and Unicode
    const char *mixed = "Hello世界";
    assert(unicode_strlen(mixed) == 7); // 5 ASCII + 2 CJK
    
    printf("  ✓ String operations tests passed\n");
}

// Test emoji detection
void test_emoji(void) {
    printf("Testing emoji detection...\n");
    
    // Common emoji codepoints
    assert(unicode_is_emoji(0x1F600)); // 😀
    assert(unicode_is_emoji(0x1F601)); // 😁
    
    // Non-emoji
    assert(!unicode_is_emoji('A'));
    assert(!unicode_is_emoji(0x4E2D)); // 中
    
    printf("  ✓ Emoji detection tests passed\n");
}

// Test math symbols
void test_math_symbols(void) {
    printf("Testing math symbols...\n");
    
    // Common math symbols
    assert(unicode_is_math_symbol(0x2200)); // ∀
    assert(unicode_is_math_symbol(0x2203)); // ∃
    assert(unicode_is_math_symbol(0x2208)); // ∈
    
    // Non-math symbols
    assert(!unicode_is_math_symbol('A'));
    assert(!unicode_is_math_symbol('0'));
    
    printf("  ✓ Math symbol tests passed\n");
}

// Display statistics
void display_statistics(void) {
    printf("\n=== Unicode Data Statistics ===\n");
    printf("Basic data size:     %zu bytes\n", sizeof(unicode_basic_data));
    printf("Math data size:      %zu bytes\n", sizeof(unicode_math_data));
    printf("Props data size:     %zu bytes\n", sizeof(unicode_props_data));
    printf("Emoji data size:     %zu bytes\n", sizeof(unicode_emoji_data));
    printf("Collation data size: %zu bytes\n", sizeof(unicode_collation_data));
    printf("IDNA data size:      %zu bytes\n", sizeof(unicode_idna_data));
    printf("Han data size:       %zu bytes\n", sizeof(unicode_han_data));
    
    size_t total = sizeof(unicode_basic_data) + sizeof(unicode_math_data) +
                   sizeof(unicode_props_data) + sizeof(unicode_emoji_data) +
                   sizeof(unicode_collation_data) + sizeof(unicode_idna_data) +
                   sizeof(unicode_han_data);
    
    printf("Total embedded size: %zu bytes (%.2f MB)\n", total, total / (1024.0 * 1024.0));
    printf("===============================\n\n");
}

int main(void) {
    printf("BDI Unicode Data Pipeline Test Suite\n");
    printf("=====================================\n\n");
    
    // Display statistics
    display_statistics();
    
    // Initialize Unicode tables
    printf("Initializing Unicode tables...\n");
    if (!unicode_tables_init()) {
        fprintf(stderr, "Failed to initialize Unicode tables\n");
        return 1;
    }
    printf("  ✓ Unicode tables initialized\n\n");
    
    // Run tests
    test_char_properties();
    test_case_conversion();
    test_utf8_conversion();
    test_string_operations();
    test_emoji();
    test_math_symbols();
    
    // Cleanup
    unicode_tables_cleanup();
    
    printf("\n=====================================\n");
    printf("All tests passed! ✓\n");
    printf("Unicode 17.0.0 data pipeline is working correctly.\n");
    
    return 0;
}
