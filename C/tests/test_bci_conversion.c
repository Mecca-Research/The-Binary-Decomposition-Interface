
// BCI Conversion Tests
#include "../bci/bci_conversion.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    printf("Testing %s... ", name); \
    fflush(stdout);

#define PASS() \
    printf("PASS\n"); \
    tests_passed++;

void test_binary_to_decimal(void) {
    TEST("binary_to_decimal_str");
    
    char buffer[64];
    
    binary_to_decimal_str(0, buffer, sizeof(buffer));
    assert(strcmp(buffer, "0") == 0);
    
    binary_to_decimal_str(123, buffer, sizeof(buffer));
    assert(strcmp(buffer, "123") == 0);
    
    binary_to_decimal_str(0xFFFFFFFFFFFFFFFFULL, buffer, sizeof(buffer));
    assert(strcmp(buffer, "18446744073709551615") == 0);
    
    PASS();
}

void test_decimal_to_binary(void) {
    TEST("decimal_str_to_binary");
    
    uint64_t value;
    
    assert(decimal_str_to_binary("0", &value) && value == 0);
    assert(decimal_str_to_binary("123", &value) && value == 123);
    assert(decimal_str_to_binary("18446744073709551615", &value) && value == 0xFFFFFFFFFFFFFFFFULL);
    
    assert(!decimal_str_to_binary("", &value));
    assert(!decimal_str_to_binary("abc", &value));
    
    PASS();
}

void test_binary_to_hex(void) {
    TEST("binary_to_hex_str");
    
    char buffer[64];
    
    binary_to_hex_str(0, buffer, sizeof(buffer), false);
    assert(strcmp(buffer, "0x0") == 0);
    
    binary_to_hex_str(0x123, buffer, sizeof(buffer), false);
    assert(strcmp(buffer, "0x123") == 0);
    
    binary_to_hex_str(0xDEADBEEF, buffer, sizeof(buffer), true);
    assert(strcmp(buffer, "0xDEADBEEF") == 0);
    
    PASS();
}

void test_hex_to_binary(void) {
    TEST("hex_str_to_binary");
    
    uint64_t value;
    
    assert(hex_str_to_binary("0", &value) && value == 0);
    assert(hex_str_to_binary("0x123", &value) && value == 0x123);
    assert(hex_str_to_binary("DEADBEEF", &value) && value == 0xDEADBEEF);
    assert(hex_str_to_binary("0xdeadbeef", &value) && value == 0xDEADBEEF);
    
    assert(!hex_str_to_binary("", &value));
    assert(!hex_str_to_binary("xyz", &value));
    
    PASS();
}

void test_binary_to_binstr(void) {
    TEST("binary_to_binstr");
    
    char buffer[128];
    
    binary_to_binstr(0, buffer, sizeof(buffer), 8);
    assert(strcmp(buffer, "0b00000000") == 0);
    
    binary_to_binstr(5, buffer, sizeof(buffer), 8);
    assert(strcmp(buffer, "0b00000101") == 0);
    
    binary_to_binstr(0xFF, buffer, sizeof(buffer), 8);
    assert(strcmp(buffer, "0b11111111") == 0);
    
    PASS();
}

void test_binstr_to_binary(void) {
    TEST("binstr_to_binary");
    
    uint64_t value;
    
    assert(binstr_to_binary("0", &value) && value == 0);
    assert(binstr_to_binary("101", &value) && value == 5);
    assert(binstr_to_binary("0b101", &value) && value == 5);
    assert(binstr_to_binary("11111111", &value) && value == 0xFF);
    
    assert(!binstr_to_binary("", &value));
    assert(!binstr_to_binary("102", &value));
    
    PASS();
}

void test_binary_to_octal(void) {
    TEST("binary_to_octal_str");
    
    char buffer[64];
    
    binary_to_octal_str(0, buffer, sizeof(buffer));
    assert(strcmp(buffer, "00") == 0);
    
    binary_to_octal_str(8, buffer, sizeof(buffer));
    assert(strcmp(buffer, "010") == 0);
    
    binary_to_octal_str(0777, buffer, sizeof(buffer));
    assert(strcmp(buffer, "0777") == 0);
    
    PASS();
}

void test_octal_to_binary(void) {
    TEST("octal_str_to_binary");
    
    uint64_t value;
    
    assert(octal_str_to_binary("0", &value) && value == 0);
    assert(octal_str_to_binary("010", &value) && value == 8);
    assert(octal_str_to_binary("0777", &value) && value == 0777);
    
    assert(!octal_str_to_binary("", &value));
    assert(!octal_str_to_binary("8", &value));
    
    PASS();
}

void test_formatted_binstr(void) {
    TEST("binary_to_binstr_formatted");
    
    char buffer[128];
    
    binary_to_binstr_formatted(0xAA, buffer, sizeof(buffer), 8, '_', 4);
    assert(strcmp(buffer, "0b1010_1010") == 0);
    
    binary_to_binstr_formatted(0xFFFF, buffer, sizeof(buffer), 16, '_', 4);
    assert(strcmp(buffer, "0b1111_1111_1111_1111") == 0);
    
    PASS();
}

int main(void) {
    printf("=== BCI Conversion Tests ===\n\n");
    
    test_binary_to_decimal();
    test_decimal_to_binary();
    test_binary_to_hex();
    test_hex_to_binary();
    test_binary_to_binstr();
    test_binstr_to_binary();
    test_binary_to_octal();
    test_octal_to_binary();
    test_formatted_binstr();
    
    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
}
