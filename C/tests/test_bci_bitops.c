
// BCI Bit Operations Tests
#include "../bci/bci_bitops.h"
#include <stdio.h>
#include <assert.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    printf("Testing %s... ", name); \
    fflush(stdout);

#define PASS() \
    printf("PASS\n"); \
    tests_passed++;

void test_bit_operations(void) {
    TEST("bit_set/clear/toggle/test");
    
    uint64_t value = 0;
    
    value = bit_set(value, 0);
    assert(value == 1);
    assert(bit_test(value, 0));
    
    value = bit_set(value, 5);
    assert(value == 33);
    assert(bit_test(value, 5));
    
    value = bit_clear(value, 0);
    assert(value == 32);
    assert(!bit_test(value, 0));
    
    value = bit_toggle(value, 5);
    assert(value == 0);
    
    PASS();
}

void test_popcount(void) {
    TEST("popcount");
    
    assert(popcount_u32(0) == 0);
    assert(popcount_u32(1) == 1);
    assert(popcount_u32(7) == 3);
    assert(popcount_u32(0xFF) == 8);
    assert(popcount_u32(0xFFFFFFFF) == 32);
    
    assert(popcount_u64(0) == 0);
    assert(popcount_u64(1) == 1);
    assert(popcount_u64(0xFFFFFFFFFFFFFFFFULL) == 64);
    
    PASS();
}

void test_clz(void) {
    TEST("clz (count leading zeros)");
    
    assert(clz_u32(0) == 32);
    assert(clz_u32(1) == 31);
    assert(clz_u32(0x80000000) == 0);
    assert(clz_u32(0x00000001) == 31);
    
    assert(clz_u64(0) == 64);
    assert(clz_u64(1) == 63);
    assert(clz_u64(0x8000000000000000ULL) == 0);
    
    PASS();
}

void test_ctz(void) {
    TEST("ctz (count trailing zeros)");
    
    assert(ctz_u32(0) == 32);
    assert(ctz_u32(1) == 0);
    assert(ctz_u32(2) == 1);
    assert(ctz_u32(8) == 3);
    assert(ctz_u32(0x80000000) == 31);
    
    assert(ctz_u64(0) == 64);
    assert(ctz_u64(1) == 0);
    assert(ctz_u64(0x8000000000000000ULL) == 63);
    
    PASS();
}

void test_ffs(void) {
    TEST("ffs (find first set)");
    
    assert(ffs_u32(0) == 0);
    assert(ffs_u32(1) == 1);
    assert(ffs_u32(2) == 2);
    assert(ffs_u32(8) == 4);
    
    assert(ffs_u64(0) == 0);
    assert(ffs_u64(1) == 1);
    assert(ffs_u64(0x8000000000000000ULL) == 64);
    
    PASS();
}

void test_parity(void) {
    TEST("parity");
    
    assert(parity_u32(0) == 0);
    assert(parity_u32(1) == 1);
    assert(parity_u32(3) == 0); // 2 bits set
    assert(parity_u32(7) == 1); // 3 bits set
    
    assert(parity_u64(0) == 0);
    assert(parity_u64(1) == 1);
    
    PASS();
}

void test_bit_reverse(void) {
    TEST("bit_reverse");
    
    uint32_t v1 = 0x12345678;
    uint32_t r1 = bit_reverse_u32(v1);
    assert(bit_reverse_u32(r1) == v1); // Double reverse
    
    uint64_t v2 = 0x123456789ABCDEF0ULL;
    uint64_t r2 = bit_reverse_u64(v2);
    assert(bit_reverse_u64(r2) == v2);
    
    PASS();
}

void test_bswap(void) {
    TEST("bswap (byte swap)");
    
    assert(bswap_u16(0x1234) == 0x3412);
    assert(bswap_u32(0x12345678) == 0x78563412);
    assert(bswap_u64(0x123456789ABCDEF0ULL) == 0xF0DEBC9A78563412ULL);
    
    PASS();
}

int main(void) {
    printf("=== BCI Bit Operations Tests ===\n\n");
    
    test_bit_operations();
    test_popcount();
    test_clz();
    test_ctz();
    test_ffs();
    test_parity();
    test_bit_reverse();
    test_bswap();
    
    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
}
