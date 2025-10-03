
// BCI Arithmetic Tests
#include "../bci/bci_arithmetic.h"
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

#define FAIL(msg) \
    printf("FAIL: %s\n", msg); \
    tests_failed++;

void test_binary_add_u8(void) {
    TEST("binary_add_u8");
    
    BinaryAddResult r1 = binary_add_u8(100, 50, false);
    assert(r1.sum == 150 && !r1.carry);
    
    BinaryAddResult r2 = binary_add_u8(200, 100, false);
    assert(r2.sum == 44 && r2.carry); // Overflow
    
    BinaryAddResult r3 = binary_add_u8(255, 1, false);
    assert(r3.sum == 0 && r3.carry);
    
    BinaryAddResult r4 = binary_add_u8(100, 50, true);
    assert(r4.sum == 151 && !r4.carry);
    
    PASS();
}

void test_binary_add_u16(void) {
    TEST("binary_add_u16");
    
    BinaryAddResult r1 = binary_add_u16(1000, 500, false);
    assert(r1.sum == 1500 && !r1.carry);
    
    BinaryAddResult r2 = binary_add_u16(60000, 10000, false);
    assert(r2.sum == 4464 && r2.carry); // Overflow
    
    PASS();
}

void test_binary_add_u32(void) {
    TEST("binary_add_u32");
    
    BinaryAddResult r1 = binary_add_u32(1000000, 500000, false);
    assert(r1.sum == 1500000 && !r1.carry);
    
    BinaryAddResult r2 = binary_add_u32(0xFFFFFFFF, 1, false);
    assert(r2.sum == 0 && r2.carry);
    
    PASS();
}

void test_binary_add_u64(void) {
    TEST("binary_add_u64");
    
    BinaryAddResult r1 = binary_add_u64(1000000000ULL, 500000000ULL, false);
    assert(r1.sum == 1500000000ULL && !r1.carry);
    
    BinaryAddResult r2 = binary_add_u64(0xFFFFFFFFFFFFFFFFULL, 1, false);
    assert(r2.sum == 0 && r2.carry);
    
    PASS();
}

void test_binary_mul_u8(void) {
    TEST("binary_mul_u8");
    
    BinaryMulResult r1 = binary_mul_u8(10, 5);
    assert(r1.product_low == 50 && r1.product_high == 0 && !r1.overflow);
    
    BinaryMulResult r2 = binary_mul_u8(200, 2);
    assert(r2.product_low == 144 && r2.product_high == 1 && r2.overflow);
    
    PASS();
}

void test_binary_mul_u16(void) {
    TEST("binary_mul_u16");
    
    BinaryMulResult r1 = binary_mul_u16(100, 50);
    assert(r1.product_low == 5000 && r1.product_high == 0 && !r1.overflow);
    
    BinaryMulResult r2 = binary_mul_u16(60000, 2);
    assert(r2.product_high > 0 && r2.overflow);
    
    PASS();
}

void test_binary_mul_u32(void) {
    TEST("binary_mul_u32");
    
    BinaryMulResult r1 = binary_mul_u32(1000, 500);
    assert(r1.product_low == 500000 && r1.product_high == 0 && !r1.overflow);
    
    BinaryMulResult r2 = binary_mul_u32(0xFFFFFFFF, 2);
    assert(r2.product_high > 0 && r2.overflow);
    
    PASS();
}

void test_binary_mul_u64(void) {
    TEST("binary_mul_u64");
    
    BinaryMulResult r1 = binary_mul_u64(1000000, 500000);
    assert(r1.product_low == 500000000000ULL && r1.product_high == 0 && !r1.overflow);
    
    BinaryMulResult r2 = binary_mul_u64(0xFFFFFFFFFFFFFFFFULL, 2);
    assert(r2.product_high > 0 && r2.overflow);
    
    PASS();
}

#ifdef __SIZEOF_INT128__
void test_binary_add_u128(void) {
    TEST("binary_add_u128");
    
    uint128_t a = 1000000000000ULL;
    uint128_t b = 500000000000ULL;
    
    BinaryAddResult128 r1 = binary_add_u128(a, b, false);
    assert(r1.sum == 1500000000000ULL && !r1.carry);
    
    PASS();
}

void test_binary_mul_u128(void) {
    TEST("binary_mul_u128");
    
    uint128_t a = 1000000ULL;
    uint128_t b = 500000ULL;
    
    BinaryMulResult128 r1 = binary_mul_u128(a, b);
    assert(r1.product_low == 500000000000ULL && r1.product_high == 0 && !r1.overflow);
    
    PASS();
}
#endif

int main(void) {
    printf("=== BCI Arithmetic Tests ===\n\n");
    
    test_binary_add_u8();
    test_binary_add_u16();
    test_binary_add_u32();
    test_binary_add_u64();
    
    test_binary_mul_u8();
    test_binary_mul_u16();
    test_binary_mul_u32();
    test_binary_mul_u64();
    
#ifdef __SIZEOF_INT128__
    test_binary_add_u128();
    test_binary_mul_u128();
#endif
    
    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
}
